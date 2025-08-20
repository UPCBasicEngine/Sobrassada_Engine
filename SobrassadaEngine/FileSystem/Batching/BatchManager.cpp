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

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    for (GeometryBatch* it : opaqueBatches)
    {
        std::vector<MeshComponent*> batchMeshes;
        for (MeshComponent* mesh : meshesToRender)
        {
            GameObject* owner = mesh->GetParent();
            if (!owner || (!owner->IsGloballyEnabled() && !mesh->GetUpdateShaderStorage())) continue;

            if (mesh->GetBatch() == it) batchMeshes.push_back(mesh);
        }

        if (batchMeshes.empty()) continue;

        unsigned int program;

        if (it->GetIsSpecular())
        {
            program = it->DoApplyWind() ? App->GetShaderModule()->GetSpecularGeometryVPOPassProgram()
                                        : App->GetShaderModule()->GetSpecularGeometryPassProgram();
        }
        else
        {
            program = it->DoApplyWind() ? App->GetShaderModule()->GetMetallicGeometryVPOPassProgram()
                                        : App->GetShaderModule()->GetMetallicGeometryPassProgram();
        }

        const auto start = std::chrono::high_resolution_clock::now();

        glUseProgram(program);

        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
        glUniformBlockBinding(program, blockIdx, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        if (isWireframe) glUniform1i(glGetUniformLocation(program, "isWireframe"), 1);
        else glUniform1i(glGetUniformLocation(program, "isWireframe"), 0);

        if (it->IsAlpha()) glUniform1i(glGetUniformLocation(program, "isAlpha"), 1);
        else glUniform1i(glGetUniformLocation(program, "isAlpha"), 0);

        if (it->IsDoubleSided()) glDisable(GL_CULL_FACE);

        if (it->DoApplyWind())
        {
            if (const WindConfig* windConfig = App->GetSceneModule()->GetScene()->GetWindsConfig();
                windConfig->GetApplyWindGlobally())
            {
                const Quat windDirection = Quat::FromEulerXYZ(0, windConfig->GetWindDirection() * DEGREE_RAD_CONV, 0);
                glUniform4f(
                    glGetUniformLocation(program, "windDirection"), windDirection.x, windDirection.y, windDirection.z,
                    windDirection.w
                );
                glUniform4f(
                    glGetUniformLocation(program, "windParameters"), App->GetEngineTimer()->GetTime(),
                    windConfig->GetWindSpeed(), std::max(1.f, windConfig->GetGustFrequency()),
                    windConfig->GetGustSpeed()
                );
            }
        }

        it->ResetUpdatedOnce();
        it->Render(batchMeshes);

        if (it->IsDoubleSided()) glEnable(GL_CULL_FACE);

        const auto end                             = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = end - start;

        const unsigned int vertexCount             = it->GetVertexCount();
        const int meshTriangles                    = vertexCount / 3;
        App->GetOpenGLModule()->AddTrianglesPerSecond(meshTriangles / elapsed.count());
        App->GetOpenGLModule()->AddVerticesCount(vertexCount);
        App->GetOpenGLModule()->AddDrawCallsCount();
    }
}

void BatchManager::RenderTransparent(
    const std::vector<MeshComponent*>& meshesToRender, const unsigned int program, CameraComponent* camera
)
{
#ifdef OPTICK
    OPTICK_CATEGORY("BatchManager::RenderTransparent", Optick::Category::Rendering)
#endif

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    std::vector<MeshComponent*> batchMeshes;
    for (MeshComponent* mesh : meshesToRender)
    {
        GameObject* owner = mesh->GetParent();
        if (!owner || (!owner->IsGloballyEnabled() && !mesh->GetUpdateShaderStorage())) continue;

        batchMeshes.push_back(mesh);
    }

    if (batchMeshes.empty()) return;

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
                float distanceA =
                    (a->GetParent()->GetGlobalTransform().TranslatePart() - App->GetCameraModule()->GetCameraPosition())
                        .LengthSq();
                float distanceB =
                    (b->GetParent()->GetGlobalTransform().TranslatePart() - App->GetCameraModule()->GetCameraPosition())
                        .LengthSq();

                return distanceA > distanceB;
            }
        }
    );

    const auto start = std::chrono::high_resolution_clock::now();

    glUseProgram(program);

    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
    glUniformBlockBinding(program, blockIdx, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    GeometryBatch* currentBatch = batchMeshes[0]->GetBatch();
    std::vector<MeshComponent*> currentBatchMeshes;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    currentBatchMeshes.push_back(batchMeshes[0]);
    glEnable(GL_BLEND);

    if (batchMeshes[0]->GetResourceMaterial()->IsDoubleSided()) glDisable(GL_CULL_FACE);

    for (size_t i = 1; i < batchMeshes.size(); ++i)
    {
        MeshComponent* mesh  = batchMeshes[i];
        GeometryBatch* batch = mesh->GetBatch();
        if (batch == currentBatch)
        {
            currentBatchMeshes.push_back(mesh);
        }
        else
        {
            currentBatch->ResetUpdatedOnce();
            currentBatch->Render(currentBatchMeshes);
            currentBatchMeshes.clear();

            if (batchMeshes[i]->GetResourceMaterial()->IsDoubleSided()) glDisable(GL_CULL_FACE);
            else glEnable(GL_CULL_FACE);
            currentBatch = batch;
            currentBatchMeshes.push_back(mesh);
        }
    }

    if (!currentBatchMeshes.empty())
    {
        currentBatch->ResetUpdatedOnce();
        currentBatch->Render(currentBatchMeshes);
        currentBatchMeshes.clear();
    }

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void BatchManager::RenderShadowMap(const std::vector<MeshComponent*>& meshesToRender, unsigned int cameraUBO)
{
#ifdef OPTICK
    OPTICK_CATEGORY("BatchManager::RenderShadowMap", Optick::Category::Rendering)
#endif
    for (GeometryBatch* it : opaqueBatches)
    {
        std::vector<MeshComponent*> batchMeshes;
        for (MeshComponent* mesh : meshesToRender)
        {
            GameObject* owner = mesh->GetParent();
            if (!owner || (!owner->IsGloballyEnabled() && !mesh->GetUpdateShaderStorage())) continue;

            if (mesh->GetBatch() == it) batchMeshes.push_back(mesh);
        }

        if (batchMeshes.empty()) continue;

        const unsigned int program = App->GetShaderModule()->GetShadowMapPassProgram();

        const auto start           = std::chrono::high_resolution_clock::now();

        glUseProgram(program);

        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
        glUniformBlockBinding(program, blockIdx, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        it->ResetUpdatedOnce();
        it->Render(batchMeshes, true);

        const auto end                             = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = end - start;

        const unsigned int vertexCount             = it->GetVertexCount();
        const int meshTriangles                    = vertexCount / 3;
        App->GetOpenGLModule()->AddTrianglesPerSecond(meshTriangles / elapsed.count());
        App->GetOpenGLModule()->AddVerticesCount(vertexCount);
        App->GetOpenGLModule()->AddDrawCallsCount();
    }
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
        return CreateNewBatch(component);
    }
    else
    {
        for (GeometryBatch* it : opaqueBatches)
        {
            if (it->GetMode() == mesh->GetMode() && it->GetIsMetallic() == material->GetIsMetallicRoughness() &&
                it->GetHasBones() == component->GetHasBones() &&
                it->IsNavmeshValid() == component->GetParent()->IsNavMeshValid() &&
                it->IsAlpha() == (component->GetRenderMode() == 2) &&
                material->IsDoubleSided() == it->IsDoubleSided() && material->DoApplyWind() == it->DoApplyWind())
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
