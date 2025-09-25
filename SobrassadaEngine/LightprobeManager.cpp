#include "LightprobeManager.h"
#include "Standalone/MeshComponent.h"
#include "Scene.h"
#include "Application.h"
#include "GameObject.h"
#include "BatchManager.h"
#include "ResourcesModule.h"
#include "CameraComponent.h"
#include "SceneModule.h"
#include "EngineTimer.h"
#include <algorithm>
#include "Globals.h"
#include "glew.h"

LightprobeManager::~LightprobeManager()
{
    for (auto& probe : probes)
    {
        if (probe.cubemapTexture != 0)
        {
            glDeleteTextures(1, &probe.cubemapTexture);
        }
    }
}

void LightprobeManager::AddProbe(const float3& position, const float3& size)
{
    Lightprobe probe;
    probe.position = position;
    probe.size     = size;
    probe.needUpdate = true;
    probes.push_back(probe);
}

void LightprobeManager::RemoveProbe(int index)
{
    if (index >= 0 && index < probes.size())
    {
        if (probes[index].cubemapTexture != 0)
        {
            glDeleteTextures(1, &probes[index].cubemapTexture);
        }
        probes.erase(probes.begin() + index);
    }
}

void LightprobeManager::RenderCubemaps()
{
    if (isRenderingCubemaps) return; 

    float currentTime = App->GetEngineTimer()->GetTime();
    if (currentTime - lastRenderTime < 1000.0f) return; 

    isRenderingCubemaps = true;

    for (auto& probe : probes)
    {
        if (probe.needUpdate)
        {
            RenderSingleProbeCubemap(probe);
            probe.needUpdate = false;
        }
    }

    lastRenderTime      = currentTime;
    isRenderingCubemaps = false;
}

std::vector<GPULightprobeData> LightprobeManager::GetRelevantLightprobes(const float3& worldPos, int maxProbes)
{
    std::vector<GPULightprobeData> gpuData;
    for (int i = 0; i < probes.size() && i < maxProbes; ++i)
    {
        probes[i].cubemapIndex = i; 
        GPULightprobeData data = CreateGPUData(probes[i], probes[i].size);
        gpuData.push_back(data);
    }
    return gpuData;
}

void LightprobeManager::BindLightprobesToShader(unsigned int shaderProgram)
{
    if (shaderProgram == 0)
    {
        GLOG("Invalid shader program ID");
        return;
    }

    glUseProgram(shaderProgram);

    UpdateLightprobeSSBO();

    int validProbes = 0;
    for (int i = 0; i < probes.size() && i < 8; ++i)
    {
        if (probes[i].cubemapTexture != 0 && i < currentGPUData.size())
        {
            validProbes++;
        }
        else break;
    }

    GLint numLightprobesLoc = glGetUniformLocation(shaderProgram, "numLightprobes");
    if (numLightprobesLoc >= 0)
    {
        glUniform1i(numLightprobesLoc, validProbes);
    }

    if (validProbes == 0 || currentGPUData.empty())
    {
        GLOG("No valid lightprobes to bind");
        return;
    }

    if (lightprobeSSBO == 0)
    {
        glGenBuffers(1, &lightprobeSSBO);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightprobeSSBO);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, sizeof(GPULightprobeData) * currentGPUData.size(), currentGPUData.data(),
        GL_DYNAMIC_DRAW
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 30, lightprobeSSBO);

    
    for (int i = 0; i < validProbes; ++i)
    {
        glActiveTexture(GL_TEXTURE30 + i); 
        glBindTexture(GL_TEXTURE_CUBE_MAP, probes[i].cubemapTexture);

        char uniformName[64];
        snprintf(uniformName, sizeof(uniformName), "lightprobeCubemaps[%d]", i);
        GLint location = glGetUniformLocation(shaderProgram, uniformName);
        if (location >= 0)
        {
            glUniform1i(location, 30 + i); 
        }
        else
        {
            GLOG("Warning: uniform %s not found", uniformName);
        }
        glActiveTexture(GL_TEXTURE0);
    }

    GLOG("Successfully bound %d lightprobes to shader", validProbes);
   

}

void LightprobeManager::UpdateLightprobeSSBO()
{
    float3 cameraPos = App->GetCameraModule()->GetCameraPosition();
    currentGPUData   = GetRelevantLightprobes(cameraPos, 8);
}

void LightprobeManager::RenderSingleProbeCubemap(Lightprobe& probe)
{
  
   if (!CreateCubemapTexture(probe))
    {
        GLOG("Failed to create cubemap for lightprobe");
        return;
    }

    GLint currentFramebuffer, currentViewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);
    glGetIntegerv(GL_VIEWPORT, currentViewport);

    unsigned int cubemapFBO, depthBuffer;
    glGenFramebuffers(1, &cubemapFBO);
    glGenRenderbuffers(1, &depthBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, cubemapFBO);

    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 256, 256);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    glViewport(0, 0, 256, 256);

   
    GameObject tempGameObject(GenerateUID(), "TempLightprobeCamera");
    tempGameObject.SetPosition(probe.position);

    CameraComponent tempCamera(GenerateUID(), &tempGameObject);
    tempCamera.ChangeToPerspective();
    tempCamera.SetFov(90.0f);
    tempCamera.SetNear(0.1f);
    tempCamera.SetFar(100.0f);
    tempCamera.SetFreeCamera(true);

 
    const float3 directions[6] = {
        float3(1.0f, 0.0f, 0.0f),  // +X (Right)
        float3(-1.0f, 0.0f, 0.0f), // -X (Left)
        float3(0.0f, 1.0f, 0.0f),  // +Y (Up)
        float3(0.0f, -1.0f, 0.0f), // -Y (Down)
        float3(0.0f, 0.0f, 1.0f),  // +Z (Forward)
        float3(0.0f, 0.0f, -1.0f)  // -Z (Back)
    };

    const float3 ups[6] = {
        float3(0.0f, -1.0f, 0.0f), // +X
        float3(0.0f, -1.0f, 0.0f), // -X
        float3(0.0f, 0.0f, 1.0f),  // +Y
        float3(0.0f, 0.0f, -1.0f), // -Y
        float3(0.0f, -1.0f, 0.0f), // +Z
        float3(0.0f, -1.0f, 0.0f)  // -Z
    };

    for (int face = 0; face < 6; ++face)
    {
        tempCamera.SetCameraPosition(probe.position);
        tempCamera.SetCameraFront(directions[face].Normalized());
        tempCamera.SetCameraUp(ups[face].Normalized());

      
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, probe.cubemapTexture, 0
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            GLOG("Framebuffer not complete for face %d", face);
            continue;
        }

        
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      
        RenderSceneForCubemap(&tempCamera);
    }

    glDeleteRenderbuffers(1, &depthBuffer);
    glDeleteFramebuffers(1, &cubemapFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);
    glViewport(currentViewport[0], currentViewport[1], currentViewport[2], currentViewport[3]);

    GLOG(
        "Rendered scene to cubemap for lightprobe at (%.1f, %.1f, %.1f)", probe.position.x, probe.position.y,
        probe.position.z
    );
}


bool LightprobeManager::CreateCubemapTexture(Lightprobe& probe)
{
    if (probe.cubemapTexture != 0)
    {
        glDeleteTextures(1, &probe.cubemapTexture);
        probe.cubemapTexture = 0;
    }

    glGenTextures(1, &probe.cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, probe.cubemapTexture);

  
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 256, 256, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        GLOG("OpenGL error creating cubemap: %d", error);
        if (probe.cubemapTexture != 0)
        {
            glDeleteTextures(1, &probe.cubemapTexture);
            probe.cubemapTexture = 0;
        }
        return false;
    }

    return true;
}

float4x4 LightprobeManager::CreateCubemapViewMatrix(const float3& position, const float3& forward, const float3& up)
{
    float3 right       = forward.Cross(up).Normalized();
    float3 correctedUp = right.Cross(forward).Normalized();

    float4x4 viewMatrix;
    viewMatrix.SetIdentity();

  
    viewMatrix.Set(0, 0, right.x);
    viewMatrix.Set(0, 1, correctedUp.x);
    viewMatrix.Set(0, 2, -forward.x);
    viewMatrix.Set(1, 0, right.y);
    viewMatrix.Set(1, 1, correctedUp.y);
    viewMatrix.Set(1, 2, -forward.y);
    viewMatrix.Set(2, 0, right.z);
    viewMatrix.Set(2, 1, correctedUp.z);
    viewMatrix.Set(2, 2, -forward.z);

    
    viewMatrix.Set(0, 3, -right.Dot(position));
    viewMatrix.Set(1, 3, -correctedUp.Dot(position));
    viewMatrix.Set(2, 3, forward.Dot(position));

    return viewMatrix;
}

float4x4 LightprobeManager::CreateCubemapProjectionMatrix()
{
    float nearPlane    = 0.1f;
    float farPlane     = 100.0f;
    float fovRadians   = 90.0f * DEGREE_RAD_CONV;

    float halfFov      = fovRadians * 0.5f;
    float viewportSize = 2.0f * nearPlane * tanf(halfFov);

    return float4x4::OpenGLPerspProjRH(nearPlane, farPlane, viewportSize, viewportSize);
}

void LightprobeManager::RenderSceneForCubemap(CameraComponent* camera)
{
    Scene* scene = App->GetSceneModule()->GetScene();
    if (!scene || !camera) return;

    const auto& allObjects = scene->GetAllGameObjects();
    std::vector<MeshComponent*> meshesToRender;

    for (const auto& [uid, gameObject] : allObjects)
    {
        if (!gameObject || !gameObject->IsGloballyEnabled()) continue;

        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh && mesh->IsEffectivelyEnabled() && mesh->GetBatch())
        {
            meshesToRender.push_back(mesh);
        }
    }

    if (meshesToRender.empty()) return;

    BatchManager* batchManager = App->GetResourcesModule()->GetBatchManager();
    if (batchManager)
    {
        batchManager->Render(meshesToRender, camera, false);
    }
}

GPULightprobeData LightprobeManager::CreateGPUData(const Lightprobe& probe, const float3& size)
{
    GPULightprobeData data {};

    data.positionAndInfluence = float4(probe.position.x, probe.position.y, probe.position.z, 1.0f);

    float3 boundsMin          = probe.position - size;
    float3 boundsMax          = probe.position + size;
    float fadeDistance        = sqrtf(size.x * size.x + size.y * size.y + size.z * size.z) * 0.2f;

    data.boundsMin            = float4(boundsMin.x, boundsMin.y, boundsMin.z, fadeDistance);
    data.boundsMax            = float4(boundsMax.x, boundsMax.y, boundsMax.z, (float)probe.cubemapIndex);

    
    float3 proxySize          = size * 0.8f;
    float3 proxyMin           = probe.position - proxySize;
    float3 proxyMax           = probe.position + proxySize;

    data.proxyMin             = float4(proxyMin.x, proxyMin.y, proxyMin.z, 0.0f);
    data.proxyMax             = float4(proxyMax.x, proxyMax.y, proxyMax.z, 0.0f);

    return data;
}
