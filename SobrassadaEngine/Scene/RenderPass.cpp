#include "RenderPass.h"
#include "Application.h"
#include "BatchManager.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "DebugDrawModule.h"
#include "Framebuffer.h"
#include "GBuffer.h"
#include "GameObject.h"
#include "LightsConfig.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourcesModule.h"
#include "ShaderModule.h"
#include "Standalone/DecalComponent.h"
#include "Standalone/Lights/DirectionalLightComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/TrailComponent.h"

#ifdef OPTICK
#include "optick.h"
#endif

#include <glew.h>

RenderPass::RenderPass()
{
    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowResolution, shadowResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        GLOG("Error: Shadow framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    constexpr float cubeVertices[] = {-0.5f, -0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, -0.5f, 0.5f,
                                      -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f};

    constexpr unsigned int cubeIndices[] = {0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7, 4, 5, 1, 1, 0, 4,
                                            3, 2, 6, 6, 7, 3, 1, 5, 6, 6, 2, 1, 4, 0, 3, 3, 7, 4};

    glGenVertexArrays(1, &decalVAO);
    glGenBuffers(1, &decalVBO);
    glGenBuffers(1, &decalEBO);

    glBindVertexArray(decalVAO);

    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, decalEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

RenderPass::~RenderPass()
{
    glDeleteBuffers(1, &decalVBO);
    glDeleteBuffers(1, &decalEBO);
    glDeleteVertexArrays(1, &decalVAO);

    glDeleteTextures(1, &depthTexture);
    glDeleteFramebuffers(1, &depthFBO);

    gbuffer     = nullptr;
    framebuffer = nullptr;
}

void RenderPass::Bind() const
{
#ifndef GAME
    framebuffer->Bind();
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    glViewport(0, 0, width, height);
}

// Copy Depth to Framebuffer
void RenderPass::CopyDepth() const
{
#ifndef GAME
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

#ifndef GAME
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// Copy Depth and Stencil to Framebuffer
void RenderPass::CopyDepthStencil() const
{
#ifndef GAME
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif

    glBlitFramebuffer(
        0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST
    );

#ifndef GAME
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetFramebufferID()); // write to default framebuffer
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void RenderPass::RenderScene(
    Framebuffer* framebuff, const std::vector<GameObject*> objectsToRender, CameraComponent* camera
)
{
    gbuffer     = App->GetOpenGLModule()->GetGBuffer();
    framebuffer = framebuff;
    width       = framebuffer->GetTextureWidth();
    height      = framebuffer->GetTextureHeight();
    glViewport(0, 0, width, height);

    glEnable(GL_STENCIL_TEST);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Geometry Pass");
    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_NAVMESH_MESHES)))
        NavMeshPassRender(objectsToRender, camera);
    else GeometryPassRender(objectsToRender, camera);
    glPopDebugGroup();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "ShadowMap Pass");
    DirectionalLightComponent* light = App->GetSceneModule()->GetScene()->GetLightsConfig()->GetDirectionalLight();
    ShadowMapPassRender(camera, light, objectsToRender);
    glPopDebugGroup();

    glViewport(0, 0, width, height);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Decals Pass");
    DecalsPassRender(objectsToRender, camera);
    glPopDebugGroup();

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_GBUFFERS)))
    {
        RenderGBufferDebug(gbuffer);
        return;
    }
    else if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_DEPTH)))
    {
        RenderDepthDebug(gbuffer, camera);
        return;
    }
    else if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_SHADOWMAP)) &&
             light != nullptr)
    {
        RenderShadowMapDebug();
        return;
    }

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Lighting Pass");
    LightingPassRender(camera, gbuffer, framebuffer);
    glPopDebugGroup();

#ifdef OPTICK
    OPTICK_CATEGORY("Scene::GameObject::Render_TransparentPass", Optick::Category::Rendering)
#endif
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Transparent Pass");
    TransparentPassRender(objectsToRender, camera);
    glPopDebugGroup();
}

void RenderPass::GeometryPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const
{
    gbuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glDisable(GL_BLEND);

    BatchManager* batchManager = App->GetResourcesModule()->GetBatchManager();
    std::vector<MeshComponent*> meshesToRender;

    for (const auto& gameObject : objectsToRender)
    {
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && mesh->GetEnabled() && mesh->GetBatch() != nullptr && mesh->GetRenderMode() != 1)
            meshesToRender.push_back(mesh);
    }

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_WIREFRAME)))
    {
        App->GetOpenGLModule()->SetRenderWireframe(true);
        batchManager->Render(meshesToRender, camera, true);
        App->GetOpenGLModule()->SetRenderWireframe(false);
    }
    else batchManager->Render(meshesToRender, camera, false);

    glEnable(GL_BLEND);

    gbuffer->Unbind();
}

void RenderPass::NavMeshPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const
{
    gbuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    glDisable(GL_BLEND);

    BatchManager* batchManager = App->GetResourcesModule()->GetBatchManager();
    std::vector<MeshComponent*> navMeshesToRender;
    std::vector<MeshComponent*> nonNavMeshesToRender;

    for (const auto& gameObject : objectsToRender)
    {
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && mesh->GetEnabled() && mesh->GetBatch() != nullptr)
        {
            if (gameObject->IsNavMeshValid()) navMeshesToRender.push_back(mesh);
            else nonNavMeshesToRender.push_back(mesh);
        }
    }

    batchManager->Render(navMeshesToRender, camera, false);
    App->GetOpenGLModule()->SetRenderWireframe(true);
    batchManager->Render(nonNavMeshesToRender, camera, true);
    App->GetOpenGLModule()->SetRenderWireframe(false);

    gbuffer->Unbind();

    glEnable(GL_BLEND);
}

void CreateDepthReductionTexture(unsigned int& texture, int width, int height)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderPass::ShadowMapPassRender(
    CameraComponent* camera, DirectionalLightComponent* light, const std::vector<GameObject*>& objectsToRender
)
{
    if (light == nullptr) return;

    // Compute shader to find min/max values
    int gBufferwidth          = gbuffer->GetScreenWidth();
    int gBufferheight         = gbuffer->GetScreenHeight();

    unsigned int currentInput = gbuffer->GetDepthTexture();
    unsigned int currentOutput;
    CreateDepthReductionTexture(currentOutput, gBufferwidth, gBufferheight);

    int currentWidth                   = gBufferwidth;
    int currentHeight                  = gBufferheight;

    bool firstPass                     = true;

    unsigned int depthReductionProgram = App->GetShaderModule()->GetComputeShadowDepthProgram();
    glUseProgram(depthReductionProgram);

    while (currentWidth > 1 || currentHeight > 1)
    {
        int groupsX = (currentWidth + 7) / 8;
        int groupsY = (currentHeight + 3) / 4;

        glBindImageTexture(0, currentOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindTextureUnit(0, currentInput);

        glUniform2i(glGetUniformLocation(depthReductionProgram, "inSize"), currentWidth, currentHeight);
        glUniform1i(glGetUniformLocation(depthReductionProgram, "firstPass"), firstPass);

        glDispatchCompute(groupsX, groupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        firstPass = false;
        unsigned int newTex;
        CreateDepthReductionTexture(newTex, groupsX, groupsY);
        glBindImageTexture(0, newTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);

        if (currentInput != gbuffer->GetDepthTexture()) glDeleteTextures(1, &currentInput);
        currentInput  = currentOutput;
        currentOutput = newTex;

        currentWidth  = groupsX;
        currentHeight = groupsY;
    }

    // Last Pass to make it 1x1
    int groupsX = (currentWidth + 7) / 8;
    int groupsY = (currentHeight + 3) / 4;

    glBindImageTexture(0, currentOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindTextureUnit(0, currentInput);

    glUniform2i(glGetUniformLocation(depthReductionProgram, "inSize"), currentWidth, currentHeight);

    glDispatchCompute(groupsX, groupsY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    float minMax[4] = {0, 0, 0, 0};

    glBindTexture(GL_TEXTURE_2D, currentOutput);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, minMax);

    float minDepth = minMax[0];
    float maxDepth = minMax[1];

    glDeleteTextures(1, &currentInput);
    glDeleteTextures(1, &currentOutput);

    // Compute the near and far planes based on the min/max depth values
    float nearD;
    float farD;
    camera == nullptr ? nearD = App->GetCameraModule()->GetNearPlaneDistance() : nearD = camera->GetNearPlaneDistance();
    camera == nullptr ? farD = App->GetCameraModule()->GetFarPlaneDistance() : farD = camera->GetFarPlaneDistance();

    float S       = (-2 * farD * nearD) / (farD - nearD);
    float T       = -(nearD + farD) / (farD - nearD);

    float distMin = S / (T + minDepth);
    float distMax = S / (T + maxDepth);

    // GLOG("Final reduction size: %d, %d", currentWidth, currentHeight);
    //GLOG("%f, %f", distMin, distMax);

    camera == nullptr ? App->GetCameraModule()->SetNear(distMin) : camera->SetNear(distMin);
    camera == nullptr ? App->GetCameraModule()->SetFar(distMax) : camera->SetFar(distMax);

    // Compute light
    float3 corners[8];
    camera == nullptr ? App->GetCameraModule()->GetFrustumCorners(corners) : camera->GetFrustumCorners(corners);

    float3 sphereCenter = float3::zero;

    camera == nullptr ? sphereCenter = App->GetCameraModule()->GetCamera().CenterPoint()
                      : sphereCenter = camera->GetCameraCenter();
    float sphereRadius = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        float dist = (corners[i] - sphereCenter).Length();
        if (dist > sphereRadius) sphereRadius = dist;
    }

    float3 lightDir = light->GetDirection();
    lightDir.Normalize();
    float3 lightUp = -lightDir.Cross(float3(1.0, 0.0, 0.0));
    lightUp.Normalize();
    float3 lightRight = lightUp.Cross(lightDir);
    lightRight.Normalize();

    Frustum shadowfrustum;

    shadowfrustum.type               = FrustumType::OrthographicFrustum;
    shadowfrustum.pos                = sphereCenter + lightDir * sphereRadius;
    shadowfrustum.front              = lightDir;
    shadowfrustum.up                 = lightUp;
    shadowfrustum.orthographicWidth  = sphereRadius * 2.0f;
    shadowfrustum.orthographicHeight = sphereRadius * 2.0f;
    shadowfrustum.nearPlaneDistance  = 0.1f;
    shadowfrustum.farPlaneDistance   = sphereRadius * 2.0f;

    CameraMatrices lightmatrices;
    lightmatrices.viewMatrix       = shadowfrustum.ViewMatrix();
    lightmatrices.projectionMatrix = shadowfrustum.ProjectionMatrix();
    lightView                      = shadowfrustum.ViewMatrix();
    lightProj                      = shadowfrustum.ProjectionMatrix();

    // DebugDrawModule* debugdraw     = App->GetDebugDrawModule();
    // debugdraw->DrawFrustrum(lightProj, lightView);

    unsigned int ubo               = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraMatrices), &lightmatrices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    BatchManager* batchManager = App->GetResourcesModule()->GetBatchManager();
    std::vector<MeshComponent*> meshesToRender;

    FrustumPlanes lightFrustum;
    lightFrustum.UpdateFrustumPlanes(lightView, lightProj);
    std::vector<GameObject*> shadowObjectsToRender;
    App->GetSceneModule()->GetScene()->CheckObjectsToRender(shadowObjectsToRender, lightFrustum);

    for (const auto& gameObject : shadowObjectsToRender)
    {
        MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
        if (mesh != nullptr && mesh->GetEnabled() && mesh->GetBatch() != nullptr && mesh->GetRenderMode() != 1 && mesh->GetProduceShadows())
            meshesToRender.push_back(mesh);
    }

    glViewport(0, 0, shadowResolution, shadowResolution);

    camera == nullptr ? App->GetCameraModule()->SetNear(nearD) : camera->SetNear(nearD);
    camera == nullptr ? App->GetCameraModule()->SetFar(farD) : camera->SetFar(farD);

    batchManager->RenderShadowMap(meshesToRender, ubo);

    glDeleteBuffers(1, &ubo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass::DecalsPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const
{
    gbuffer->Bind();

    std::vector<DecalComponent*> decalsToRender;
    std::unordered_map<UID, std::vector<DecalComponent*>> groupedDecals;

    for (const auto& gameObject : objectsToRender)
    {
        DecalComponent* decal = gameObject->GetComponent<DecalComponent*>();

        if (decal == nullptr) continue;
        if (decal->GetResourceMaterial() == nullptr) continue;
        if (!decal->IsEffectivelyEnabled()) continue;

        const UID uid = decal->GetResourceMaterial()->GetUID();
        groupedDecals[uid].push_back(decal);
    }

    if (groupedDecals.empty()) return;

    const unsigned int program = App->GetShaderModule()->GetDecalProgram();

    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);
    glUniform1i(glGetUniformLocation(program, "positionTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);
    glUniform1i(glGetUniformLocation(program, "normalTex"), 1);

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
    glUniformBlockBinding(program, blockIdx, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);

    for (const auto& [uid, decals] : groupedDecals)
    {
        
        const uint64_t dhandle = decals[0]->GetResourceMaterial()->GetMaterial().diffuseTex;
        glUniformHandleui64ARB(glGetUniformLocation(program, "decalAlbedoTex"), dhandle);

        if (decals[0]->GetResourceMaterial()->GetMaterial().hasMetallic)
        {
            glUniform1i(glGetUniformLocation(program, "hasMetallic"), 1);

            const uint64_t mhandle = decals[0]->GetResourceMaterial()->GetMaterial().metallicTex;
            glUniformHandleui64ARB(glGetUniformLocation(program, "decalMetallicTex"), mhandle);
        }
        else if (decals[0]->GetResourceMaterial()->GetMaterial().hasSpecular)
        {
            glUniform1i(glGetUniformLocation(program, "hasMetallic"), 1);

            const uint64_t mhandle = decals[0]->GetResourceMaterial()->GetMaterial().specularTex;
            glUniformHandleui64ARB(glGetUniformLocation(program, "decalMetallicTex"), mhandle);
        }
        else glUniform1i(glGetUniformLocation(program, "hasMetallic"), 0);

        glUniform1i(glGetUniformLocation(program, "hasNormal"), decals[0]->GetResourceMaterial()->HasNormal() ? 1 : 0);

        const uint64_t nhandle = decals[0]->GetResourceMaterial()->GetMaterial().normalTex;
        glUniformHandleui64ARB(glGetUniformLocation(program, "decalNormalTex"), nhandle);

        std::vector<DecalModels> models;
        models.reserve(decals.size());

        for (const auto& decal : decals)
        {
            if (!decal->GetEnabled()) continue;
            float4x4 model    = decal->GetParent()->GetGlobalTransform();
            float4x4 invModel = model.Inverted();

            models.push_back({model, invModel});
        }

        if (models.empty()) continue;

        GLuint decalSSBO;
        glGenBuffers(1, &decalSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, decalSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DecalModels) * models.size(), models.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, decalSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        /*
        We cant check if we are inside of the decal with instancing (the camera is far away, so we should'nt have any
        problem)

        float3 cameraPos;
        if (camera == nullptr) cameraPos = App->GetCameraModule()->GetCameraPosition();
        else cameraPos = camera->GetCameraPosition();
        float3 localCameraPos = (invModel * float4(cameraPos, 1.0f)).xyz();

        bool insideDecalBox =
            abs(localCameraPos.x) <= 0.5f && abs(localCameraPos.y) <= 0.5f && abs(localCameraPos.z) <= 0.5f;


        if (insideDecalBox)
        {
            glDisable(GL_DEPTH_TEST);
            glFrontFace(GL_CW);
        }*/

        glBindVertexArray(decalVAO);

        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, (GLsizei)models.size());

        glBindVertexArray(0);

        glDeleteBuffers(1, &decalSSBO);
    }

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    gbuffer->Unbind();
}

void RenderPass::LightingPassRender(CameraComponent* camera, GBuffer* gbuffer, Framebuffer* framebuffer) const
{
    Bind();

    // SKYBOX
    if (!App->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_WIREFRAME))
    {
        float4x4 projection;
        float4x4 view;

        if (camera == nullptr)
            App->GetSceneModule()->GetScene()->GetLightsConfig()->RenderSkybox(
                App->GetCameraModule()->GetProjectionMatrix(), App->GetCameraModule()->GetViewMatrix()
            );
        else
        {
            bool change = false;
            // Cubemap does not support Ortographic projection
            if (camera->GetFrustumType() == 1)
            {
                change = true;
                camera->ChangeToPerspective();
            }
            App->GetSceneModule()->GetScene()->GetLightsConfig()->RenderSkybox(
                camera->GetProjectionMatrix(), camera->GetViewMatrix()
            );
            if (change) camera->ChangeToOrtographic();
        }
    }

    // COPYING DEPTH BUFFER AND STENCIL FROM GBUFFER TO RENDER FRAMEBUFFER
    // TODO CHECK IF GAME RELEASE TO RENDER TO DEFAULT BUFFER INSTEAD OF FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->gBufferObject);

    CopyDepthStencil();

    // SETTING STENCIL TEST FOR ONLY RENDER TO GBUFFER FRAGMENTS WRITES
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0xFF);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gbuffer->specularTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    App->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();

    unsigned int lightingPassProgram = App->GetShaderModule()->GetLightingPassProgram();

    glUseProgram(lightingPassProgram);

    float3 cameraPos;
    if (camera == nullptr) cameraPos = App->GetCameraModule()->GetCameraPosition();
    else cameraPos = camera->GetCameraPosition();

    glUniformMatrix4fv(glGetUniformLocation(lightingPassProgram, "viewLight"), 1, GL_TRUE, lightView.ptr());
    glUniformMatrix4fv(glGetUniformLocation(lightingPassProgram, "projLight"), 1, GL_TRUE, lightProj.ptr());

    DirectionalLightComponent* light = App->GetSceneModule()->GetScene()->GetLightsConfig()->GetDirectionalLight();
    if (light != nullptr)
    {
        float3 shadowTint = light->GetShadowTint();
        glUniform3f(glGetUniformLocation(lightingPassProgram, "shadowTint"), shadowTint.x, shadowTint.y, shadowTint.z);
    }

    glUniform3fv(glGetUniformLocation(lightingPassProgram, "cameraPos"), 1, &cameraPos[0]);

    App->GetOpenGLModule()->DrawArrays(GL_TRIANGLES, 0, 3);

    glDisable(GL_STENCIL_TEST);

    // COPYING DEPTH BUFFER FROM GBUFFER TO RENDER FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->gBufferObject);

    CopyDepth();
}

void RenderPass::TransparentPassRender(const std::vector<GameObject*>& objectsToRender, CameraComponent* camera) const
{
    Bind();

    BatchManager* batchManager = App->GetResourcesModule()->GetBatchManager();

    const unsigned int program = App->GetShaderModule()->GetTransparentPassProgram();

    glUseProgram(program);

    App->GetSceneModule()->GetScene()->GetLightsConfig()->SetLightsShaderData();

    float3 cameraPos;
    if (camera == nullptr) cameraPos = App->GetCameraModule()->GetCameraPosition();
    else cameraPos = camera->GetCameraPosition();

    glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, &cameraPos[0]);

    if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_NAVMESH_MESHES)))
    {
        std::vector<MeshComponent*> navmeshesToRender;
        std::vector<MeshComponent*> nonnavmeshesToRender;

        for (const auto& gameObject : objectsToRender)
        {
            MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
            if (mesh != nullptr && mesh->GetEnabled() && mesh->GetBatch() != nullptr && mesh->GetRenderMode() == 1)
            {
                if (gameObject->IsNavMeshValid()) navmeshesToRender.push_back(mesh);
                else nonnavmeshesToRender.push_back(mesh);
            }
        }

        glUniform1i(glGetUniformLocation(program, "isWireframe"), 0);
        batchManager->RenderTransparent(navmeshesToRender, camera);
        glUniform1i(glGetUniformLocation(program, "isWireframe"), 1);
        App->GetOpenGLModule()->SetRenderWireframe(true);
        batchManager->RenderTransparent(nonnavmeshesToRender, camera);
        App->GetOpenGLModule()->SetRenderWireframe(false);
    }

    else
    {
        std::vector<MeshComponent*> meshesToRender;
        std::vector<TrailComponent*> trailsToRender;

        for (const auto& gameObject : objectsToRender)
        {
            MeshComponent* mesh = gameObject->GetComponent<MeshComponent*>();
            if (mesh != nullptr && mesh->GetEnabled() && mesh->GetBatch() != nullptr && mesh->GetRenderMode() == 1)
                meshesToRender.push_back(mesh);

            TrailComponent* trail = gameObject->GetComponent<TrailComponent*>();
            if (trail != nullptr && trail->GetEnabled()) trailsToRender.push_back(trail);
        }

        if (App->GetDebugDrawModule()->GetDebugOptionValue(static_cast<int>(DebugOptions::RENDER_WIREFRAME)))
        {
            glUniform1i(glGetUniformLocation(program, "isWireframe"), 1);
            App->GetOpenGLModule()->SetRenderWireframe(true);
            batchManager->RenderTransparent(meshesToRender, camera);
            App->GetOpenGLModule()->SetRenderWireframe(false);
        }
        else
        {
            glUniform1i(glGetUniformLocation(program, "isWireframe"), 0);
            batchManager->RenderTransparent(meshesToRender, camera);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_CULL_FACE);
            glDepthMask(GL_FALSE);

            const unsigned int program = App->GetShaderModule()->GetTrailProgram();
            glUseProgram(program);

            unsigned int cameraUBO;
            if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
            else cameraUBO = camera->GetUbo();

            glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
            unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
            glUniformBlockBinding(program, blockIdx, 0);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            for (const auto& trail : trailsToRender)
                trail->Render(0);
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void RenderPass::RenderGBufferDebug(GBuffer* gbuffer) const
{
    Bind();

    const unsigned int program = App->GetShaderModule()->GetQuadProgram();
    glUseProgram(program);

    GLint loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    // Top-left: Diffuse
    glViewport(0, height / 2, width / 2, height / 2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->diffuseTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Top-right: Specular
    glViewport(width / 2, height / 2, width / 2, height / 2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->specularTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Bottom-left: Position
    glViewport(0, 0, width / 2, height / 2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->positionTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Bottom-right: Normal
    glViewport(width / 2, 0, width / 2, height / 2);
    glBindTexture(GL_TEXTURE_2D, gbuffer->normalTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glViewport(0, 0, width, height);
}

void RenderPass::RenderDepthDebug(GBuffer* gbuffer, CameraComponent* camera) const
{
    Bind();

    const unsigned int program = App->GetShaderModule()->GetLinearDepthProgram();
    glUseProgram(program);

    GLint loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuffer->GetDepthTexture());

    float nearPlane;
    float farPlane;
    if (camera == nullptr)
    {
        nearPlane = App->GetCameraModule()->GetNearPlaneDistance();
        farPlane  = 100; // Far plane is too much
    }
    else
    {
        nearPlane = camera->GetNearPlaneDistance();
        farPlane  = camera->GetFarPlaneDistance();
    }

    glUniform1f(glGetUniformLocation(program, "nearPlane"), nearPlane);
    glUniform1f(glGetUniformLocation(program, "farPlane"), farPlane);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void RenderPass::RenderShadowMapDebug() const
{
    Bind();

    const unsigned int program = App->GetShaderModule()->GetDepthProgram();
    glUseProgram(program);

    GLint loc = glGetUniformLocation(program, "u_Texture");
    glUniform1i(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}