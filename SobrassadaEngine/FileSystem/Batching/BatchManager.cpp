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
    for (GeometryBatch* it : batches)
    {
        delete it;
    }
    batches.clear();
    batches.shrink_to_fit();
}

void BatchManager::RemoveBatch(GeometryBatch* removeBatch)
{
    for (int i = 0; i < batches.size(); i++)
    {
        if (batches[i] == removeBatch)
        {
            delete batches[i];
            batches.erase(batches.begin() + i);
            break;
        }
    }
}

void BatchManager::LoadData()
{
    for (GeometryBatch* it : batches)
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

    for (GeometryBatch* it : batches)
    {
        std::vector<MeshComponent*> batchMeshes;
        for (MeshComponent* mesh : meshesToRender)
        {
            GameObject* owner = mesh->GetParent();
            if (!owner || !owner->IsGloballyEnabled()) continue;

            if (mesh->GetBatch() == it) batchMeshes.push_back(mesh);
        }

        if (batchMeshes.empty()) continue;

        const unsigned int program = it->GetIsMetallic() ? App->GetShaderModule()->GetMetallicGeometryPassProgram()
                                                         : App->GetShaderModule()->GetSpecularGeometryPassProgram();

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

GeometryBatch* BatchManager::RequestBatch(const MeshComponent* component)
{
    if (batches.empty())
    {
        return CreateNewBatch(component);
    }

    const ResourceMesh* mesh         = component->GetResourceMesh();
    const ResourceMaterial* material = component->GetResourceMaterial();

    for (GeometryBatch* it : batches)
    {
        if (it->GetMode() == mesh->GetMode() && it->GetIsMetallic() == material->GetIsMetallicRoughness() &&
            it->GetHasBones() == component->GetHasBones() && it->IsNavmeshValid() == component->GetParent()->IsNavMeshValid())
        {
            return it;
        }
    }

    return CreateNewBatch(component);
}

GeometryBatch* BatchManager::CreateNewBatch(const MeshComponent* component)
{
    GeometryBatch* newBatch = new GeometryBatch(component);
    batches.push_back(newBatch);
    return newBatch;
}
