#include "BatchManager.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "GameObject.h"
#include "GeometryBatch.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ShaderModule.h"
#include "Standalone/MeshComponent.h"

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

void BatchManager::Render(const std::vector<MeshComponent*>& meshesToRender, CameraComponent* camera)
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
            if (!owner || !owner->IsGloballyEnabled()) continue;

            if (mesh->GetBatch() == it) batchMeshes.push_back(mesh);
        }

        if (batchMeshes.empty()) continue;

        const unsigned int program = it->GetIsSpecular() ? App->GetShaderModule()->GetSpecularGeometryPassProgram()
                                                         : App->GetShaderModule()->GetMetallicGeometryPassProgram();

        const auto start           = std::chrono::high_resolution_clock::now();

        glUseProgram(program);

        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        unsigned int blockIdx = glGetUniformBlockIndex(program, "CameraMatrices");
        glUniformBlockBinding(program, blockIdx, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        it->ResetUpdatedOnce();
        it->Render(batchMeshes);

        const auto end                             = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = end - start;

        const unsigned int vertexCount             = it->GetVertexCount();
        const int meshTriangles                    = vertexCount / 3;
        App->GetOpenGLModule()->AddTrianglesPerSecond(meshTriangles / elapsed.count());
        App->GetOpenGLModule()->AddVerticesCount(vertexCount);
        App->GetOpenGLModule()->AddDrawCallsCount();
    }
}

void BatchManager::RenderTransparent(const std::vector<MeshComponent*>& meshesToRender, CameraComponent* camera)
{
#ifdef OPTICK
    OPTICK_CATEGORY("BatchManager::Render", Optick::Category::Rendering)
#endif

    unsigned int cameraUBO;
    if (camera == nullptr) cameraUBO = App->GetCameraModule()->GetUbo();
    else cameraUBO = camera->GetUbo();

    std::vector<MeshComponent*> batchMeshes;
    for (MeshComponent* mesh : meshesToRender)
    {
        GameObject* owner = mesh->GetParent();
        if (!owner || !owner->IsGloballyEnabled()) continue;

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

    const unsigned int program = App->GetShaderModule()->GetTransparentPassProgram();

    const auto start           = std::chrono::high_resolution_clock::now();

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
    if (batchMeshes[0]->GetRenderMode() == 1) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);

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

            if (batchMeshes[0]->GetRenderMode() == 1) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
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
}

// We can change that now
GeometryBatch* BatchManager::RequestBatch(const MeshComponent* component)
{
    const bool isTransparent = component->GetRenderMode() != 0;
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
                it->IsNavmeshValid() == component->GetParent()->IsNavMeshValid())
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
