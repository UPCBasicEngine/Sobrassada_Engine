#include "BatchManager.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "EngineTimer.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"
#include "Standalone/MeshComponent.h"
#include "WindConfig.h"

#include "Math/Quat.h"
#include "Math/float3.h"
#include "glew.h"
#include <Math/MathFunc.h>
#include <algorithm>
#include <chrono>
#ifdef OPTICK
#include "optick.h"
#endif

BatchManager::BatchManager()
{
}

BatchManager::~BatchManager()
{
    UnloadAllBatches();
}

void BatchManager::UnloadAllBatches()
{
    for (GeometryBatch* it : opaqueBatches)
    {
        delete it;
    }
    opaqueBatches.clear();
    opaqueBatches.shrink_to_fit();

    for (GeometryBatch* it : transparentBatches)
    {
        delete it;
    }
    transparentBatches.clear();
    transparentBatches.shrink_to_fit();
}

void BatchManager::RemoveBatch(GeometryBatch* removeBatch)
{
    for (int i = 0; i < opaqueBatches.size(); i++)
    {
        if (opaqueBatches[i] == removeBatch)
        {
            delete opaqueBatches[i];
            opaqueBatches.erase(opaqueBatches.begin() + i);
            break;
        }
    }

    for (int i = 0; i < transparentBatches.size(); ++i)
    {
        if (transparentBatches[i] == removeBatch)
        {
            delete transparentBatches[i];
            transparentBatches.erase(transparentBatches.begin() + i);
            return;
        }
    }
}

void BatchManager::LoadData()
{
    for (GeometryBatch* it : opaqueBatches)
        it->LoadData();
    for (GeometryBatch* it : transparentBatches)
        it->LoadData();
}

void BatchManager::Render(const std::vector<MeshComponent*>& meshesToRender, CameraComponent* camera, bool isWireframe)
{
#ifdef OPTICK
    OPTICK_CATEGORY("BatchManager::Render", Optick::Category::Rendering)
#endif

#ifdef OPTICK
    OPTICK_PUSH("BatchManager::meshesToRender")
#endif
    if (meshesToRender.empty()) return;

    std::unordered_map<GeometryBatch*, std::vector<MeshComponent*>> grouped;
    grouped.reserve(meshesToRender.size());

    for (MeshComponent* mesh : meshesToRender)
    {
        if (!mesh) continue;
        GameObject* owner = mesh->GetParent();
        if (!owner || (!owner->IsGloballyEnabled() && !mesh->GetUpdateShaderStorage())) continue;

        GeometryBatch* b = mesh->GetBatch();
        if (!b) continue;

        auto& vec = grouped[b];
        if (vec.empty()) vec.reserve(8);
        vec.push_back(mesh);
    }

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    uint64_t totalTriangles  = 0;
    uint64_t totalVertices   = 0;

    const auto passStart     = std::chrono::high_resolution_clock::now();

    unsigned int lastProgram = -1;

    for (GeometryBatch* batch : opaqueBatches)
    {
#ifdef OPTICK
        OPTICK_POP();
        OPTICK_PUSH("BatchManager::opaqueMeshes")
#endif
        auto it = grouped.find(batch);
        if (it == grouped.end()) continue;

        const std::vector<MeshComponent*>& batchMeshes = it->second;
        if (batchMeshes.empty()) continue;

        unsigned int program;

        if (batch->GetIsSpecular())
        {
            program = batch->DoApplyWind() ? App->GetShaderModule()->GetSpecularGeometryVPOPassProgram()
                                           : App->GetShaderModule()->GetSpecularGeometryPassProgram();
        }
        else
        {
            program = batch->DoApplyWind() ? App->GetShaderModule()->GetMetallicGeometryVPOPassProgram()
                                           : App->GetShaderModule()->GetMetallicGeometryPassProgram();
        }

        UniformCache& u = uniformCacheMap[program];
        if (lastProgram != program)
        {
            glUseProgram(program);

            if (!u.initialized)
            {
                u.isWireframe    = glGetUniformLocation(program, "isWireframe");
                u.isAlpha        = glGetUniformLocation(program, "isAlpha");
                u.windParameters = glGetUniformLocation(program, "windParameters");
                u.windUVParams   = glGetUniformLocation(program, "windUVParameters");
                u.windAmplitudes = glGetUniformLocation(program, "windAmplitudes");
                u.windFrequency  = glGetUniformLocation(program, "windFrequency");
                u.cameraBlockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
                if (u.cameraBlockIdx != -1) glUniformBlockBinding(program, u.cameraBlockIdx, 0);
                u.initialized = true;
            }

            glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
            lastProgram = program;
        }

        if (isWireframe) glUniform1i(u.isWireframe, 1);
        else glUniform1i(u.isWireframe, 0);

        if (batch->IsAlpha()) glUniform1i(u.isAlpha, 1);
        else glUniform1i(u.isAlpha, 0);

        if (batch->IsDoubleSided()) glDisable(GL_CULL_FACE);

        if (batch->DoApplyWind())
        {
                if (const WindConfig* windConfig = App->GetSceneModule()->GetScene()->GetWindsConfig();
                    windConfig->GetApplyWindGlobally())
                {
                    glUniform4f(
                        u.windParameters, App->GetEngineTimer()->GetTime(), windConfig->GetWindSpeed(),
                        std::max(1.f, windConfig->GetGustFrequency()), windConfig->GetGustSpeed()
                    );
                    glUniform4f(
                        u.windUVParams, batch->GetVCoord0(), batch->GetVCoord1(), batch->UseCentralPivot(),
                        batch->UseWindGravity()
                    );
                    glUniform4f(
                        u.windAmplitudes, batch->GetWindXAmplitude(), batch->GetWindYAmplitude(),
                        batch->GetWindZAmplitude(), batch->UseConstantMovement()
                    );
                    glUniform4f(
                        u.windFrequency, batch->GetWindXFrequency(), batch->GetWindYFrequency(),
                        batch->GetWindZFrequency(), batch->GetWindTimeScale()
                    );
                }
        }

        batch->ResetUpdatedOnce();

        batch->Render(batchMeshes, false);

        const unsigned int vertexCount  = batch->GetVertexCount();
        totalVertices                  += vertexCount;

        totalTriangles                 += (vertexCount / 3);

#ifdef OPTICK
        OPTICK_POP();
#endif
    }

    const auto passEnd                         = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> elapsed = passEnd - passStart;

    if (elapsed.count() > 0.0f)
    {
        App->GetOpenGLModule()->AddTrianglesPerSecond(static_cast<float>(totalTriangles) / elapsed.count());
    }

    App->GetOpenGLModule()->AddVerticesCount(static_cast<int>(totalVertices));
    App->GetOpenGLModule()->AddDrawCallsCount();

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glUseProgram(0);
}

void BatchManager::RenderTransparent(
    const std::vector<MeshComponent*>& meshesToRender, const unsigned int program, CameraComponent* camera
)
{
#ifdef OPTICK
    OPTICK_CATEGORY("BatchManager::RenderTransparent", Optick::Category::Rendering)
#endif

    if (meshesToRender.empty()) return;

    std::unordered_map<GeometryBatch*, std::vector<MeshComponent*>> grouped;
    grouped.reserve(meshesToRender.size());

    for (MeshComponent* mesh : meshesToRender)
    {
        if (!mesh) continue;
        GameObject* owner = mesh->GetParent();
        if (!owner || (!owner->IsGloballyEnabled() && !mesh->GetUpdateShaderStorage())) continue;

        GeometryBatch* b = mesh->GetBatch();
        if (!b) continue;

        auto& vec = grouped[b];
        if (vec.empty()) vec.reserve(8);
        vec.push_back(mesh);
    }

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    glUseProgram(program);

    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
    glUniformBlockBinding(program, blockIdx, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    const auto passStart    = std::chrono::high_resolution_clock::now();
    uint64_t totalTriangles = 0;
    uint64_t totalVertices  = 0;

    for (GeometryBatch* batch : transparentBatches)
    {
        auto it = grouped.find(batch);
        if (it == grouped.end()) continue;

        std::vector<MeshComponent*>& batchMeshes = it->second;
        if (batchMeshes.empty()) continue;

        std::sort(
            batchMeshes.begin(), batchMeshes.end(),
            [camera](MeshComponent* a, MeshComponent* b)
            {
                if (camera != nullptr)
                {
                    float distanceA =
                        (a->GetParent()->GetGlobalTransform().TranslatePart() - camera->GetCameraPosition()).LengthSq();
                    float distanceB =
                        (b->GetParent()->GetGlobalTransform().TranslatePart() - camera->GetCameraPosition()).LengthSq();

                    return distanceA > distanceB;
                }
                else
                {
                    float distanceA = (a->GetParent()->GetGlobalTransform().TranslatePart() -
                                       App->GetCameraModule()->GetCameraPosition())
                                          .LengthSq();
                    float distanceB = (b->GetParent()->GetGlobalTransform().TranslatePart() -
                                       App->GetCameraModule()->GetCameraPosition())
                                          .LengthSq();

                    return distanceA > distanceB;
                }
            }
        );

        glEnable(GL_BLEND);
        if (batch->IsAdditive()) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GeometryBatch* currentBatch = batchMeshes[0]->GetBatch();
        std::vector<MeshComponent*> currentBatchMeshes;
        currentBatchMeshes.reserve(batchMeshes.size());

        glDisable(GL_CULL_FACE);

        const auto start = std::chrono::high_resolution_clock::now();

        for (MeshComponent* mesh : batchMeshes)
        {
            GeometryBatch* meshBatch = mesh->GetBatch();
            if (meshBatch == currentBatch)
            {
                currentBatchMeshes.push_back(mesh);
            }
            else
            {
                currentBatch->ResetUpdatedOnce();
                currentBatch->Render(currentBatchMeshes);
                currentBatchMeshes.clear();

                currentBatch = meshBatch;
                currentBatchMeshes.push_back(mesh);
            }
        }

        if (!currentBatchMeshes.empty())
        {
            currentBatch->ResetUpdatedOnce();
            currentBatch->Render(currentBatchMeshes);
        }

        const unsigned int vertexCount  = batch->GetVertexCount();
        totalVertices                  += vertexCount;
        totalTriangles                 += (vertexCount / 3);
    }

    const auto passEnd                         = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> elapsed = passEnd - passStart;
    if (elapsed.count() > 0.0f)
    {
        App->GetOpenGLModule()->AddTrianglesPerSecond(static_cast<float>(totalTriangles) / elapsed.count());
    }
    App->GetOpenGLModule()->AddVerticesCount(static_cast<int>(totalVertices));
    App->GetOpenGLModule()->AddDrawCallsCount();

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glUseProgram(0);
}

void BatchManager::RenderShadowMap(const std::vector<MeshComponent*>& meshesToRender, unsigned int cameraUBO)
{
#ifdef OPTICK
    OPTICK_CATEGORY("BatchManager::RenderShadowMap", Optick::Category::Rendering)
#endif

    if (meshesToRender.empty()) return;

    std::unordered_map<GeometryBatch*, std::vector<MeshComponent*>> grouped;
    grouped.reserve(meshesToRender.size());

    for (MeshComponent* mesh : meshesToRender)
    {
        if (!mesh) continue;
        GameObject* owner = mesh->GetParent();
        if (!owner || !owner->IsGloballyEnabled()) continue;

        GeometryBatch* b = mesh->GetBatch();
        if (!b) continue;

        auto& vec = grouped[b];
        if (vec.empty()) vec.reserve(8);
        vec.push_back(mesh);
    }

    const unsigned int program = App->GetShaderModule()->GetShadowMapPassProgram();
    glUseProgram(program);

    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
    glUniformBlockBinding(program, blockIdx, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    uint64_t totalTriangles = 0;
    uint64_t totalVertices  = 0;

    const auto passStart    = std::chrono::high_resolution_clock::now();

    for (GeometryBatch* batch : opaqueBatches)
    {
        auto it = grouped.find(batch);
        if (it == grouped.end()) continue;

        const std::vector<MeshComponent*>& batchMeshes = it->second;
        if (batchMeshes.empty()) continue;

        batch->ResetUpdatedOnce();

        batch->Render(batchMeshes, true);

        const unsigned int vertexCount  = batch->GetVertexCount();
        totalVertices                  += vertexCount;

        totalTriangles                 += (vertexCount / 3);
    }

    const auto passEnd                         = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> elapsed = passEnd - passStart;

    if (elapsed.count() > 0.0f)
    {
        App->GetOpenGLModule()->AddTrianglesPerSecond(static_cast<float>(totalTriangles) / elapsed.count());
    }

    App->GetOpenGLModule()->AddVerticesCount(static_cast<int>(totalVertices));
    App->GetOpenGLModule()->AddDrawCallsCount();

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glUseProgram(0);
}

void BatchManager::SwapBuffers()
{
    for (GeometryBatch* it : opaqueBatches)
        it->SwapBuffers();
    for (GeometryBatch* it : transparentBatches)
        it->SwapBuffers();
}

// We can change that now
GeometryBatch* BatchManager::RequestBatch(const MeshComponent* component)
{
    const bool isTransparent = component->GetRenderMode() == 1;
    if (isTransparent)
    {
        if (transparentBatches.empty()) return CreateNewBatch(component);
    }
    else if (opaqueBatches.empty()) return CreateNewBatch(component);

    const ResourceMesh* mesh         = component->GetResourceMesh();
    const ResourceMaterial* material = component->GetResourceMaterial();

    if (isTransparent)
    {
        for (GeometryBatch* it : transparentBatches)
        {
            if (it->IsAdditive() == component->GetAdditive()) return it;
            else return CreateNewBatch(component);
        }
    }
    else
    {
        for (GeometryBatch* it : opaqueBatches)
        {
            if (it->GetMode() == mesh->GetMode() && it->GetIsMetallic() == material->GetIsMetallicRoughness() &&
                it->GetHasBones() == component->GetHasBones() &&
                it->IsNavmeshValid() == component->GetParent()->IsNavMeshValid() &&
                it->IsAlpha() == (component->GetRenderMode() == 2) &&
                material->IsDoubleSided() == it->IsDoubleSided() && material->DoApplyWind() == it->DoApplyWind() &&
                Equal(material->GetVCoord0(), it->GetVCoord0()) && Equal(material->GetVCoord1(), it->GetVCoord1()) &&
                material->UseCentralPivot() == it->UseCentralPivot() &&
                material->UseWindGravity() == it->UseWindGravity() &&
                material->UseConstantMovement() == it->UseConstantMovement() &&
                Equal(material->GetWindXAmplitude(), it->GetWindXAmplitude()) &&
                Equal(material->GetWindYAmplitude(), it->GetWindYAmplitude()) &&
                Equal(material->GetWindZAmplitude(), it->GetWindZAmplitude()) &&
                Equal(material->GetWindXFrequency(), it->GetWindXFrequency()) &&
                Equal(material->GetWindYFrequency(), it->GetWindYFrequency()) &&
                Equal(material->GetWindZFrequency(), it->GetWindZFrequency()) &&
                Equal(material->GetWindTimeScale(), it->GetWindTimeScale()))
            {
                return it;
            }
        }
    }

    return CreateNewBatch(component);
}

GeometryBatch* BatchManager::CreateNewBatch(const MeshComponent* component)
{
    GeometryBatch* newBatch  = new GeometryBatch(component);
    const bool isTransparent = component->GetRenderMode() == 1;
    if (isTransparent) transparentBatches.push_back(newBatch);
    else opaqueBatches.push_back(newBatch);
    return newBatch;
}
