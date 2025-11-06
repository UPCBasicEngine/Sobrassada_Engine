#include "GeometryBatch.h"

#include "GameObject.h"
#include "Globals.h"
#include "Mesh.h"
#include "ResourceMaterial.h"
#include "ResourceMesh.h"
#include "Standalone/MeshComponent.h"
#include "WindConfig.h"

#include "glew.h"

#include <Math/Quat.h>
#ifdef OPTICK
#include "optick.h"
#endif

struct Command
{
    unsigned int count;         // Number of indices in the mesh
    unsigned int instanceCount; // Number of instances to render
    unsigned int firstIndex;    // Index offset in the EBO
    unsigned int baseVertex;    // Vertex offset in the VBO
    unsigned int baseInstance;  // Instance Index
};

GeometryBatch::GeometryBatch(const MeshComponent* component)
    : totalVertexCount(0), totalIndexCount(0), currentBufferIndex(0)
{
    mode                = component->GetResourceMesh()->GetMode();
    isSpecular          = component->GetResourceMaterial()->GetIsSpecular();
    isMetallic          = component->GetResourceMaterial()->GetIsMetallicRoughness();
    hasBones            = component->GetHasBones();
    isNavmeshValid      = component->GetParent()->IsNavMeshValid();
    isAlpha             = component->GetRenderMode() == 2;
    isDoubleSided       = component->GetResourceMaterial()->IsDoubleSided();
    additive            = component->GetAdditive();
    doApplyWind         = component->GetResourceMaterial()->DoApplyWind();
    vCoord0             = component->GetResourceMaterial()->GetVCoord0();
    vCoord1             = component->GetResourceMaterial()->GetVCoord1();
    useCentralPivot     = component->GetResourceMaterial()->UseCentralPivot();
    useWindGravity      = component->GetResourceMaterial()->UseWindGravity();
    useConstantMovement = component->GetResourceMaterial()->UseConstantMovement();
    windXAmplitude      = component->GetResourceMaterial()->GetWindXAmplitude();
    windYAmplitude      = component->GetResourceMaterial()->GetWindYAmplitude();
    windZAmplitude      = component->GetResourceMaterial()->GetWindZAmplitude();
    windXFrequency      = component->GetResourceMaterial()->GetWindXFrequency();
    windYFrequency      = component->GetResourceMaterial()->GetWindYFrequency();
    windZFrequency      = component->GetResourceMaterial()->GetWindZFrequency();
    windTimeScale       = component->GetResourceMaterial()->GetWindTimeScale();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &indirect);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenBuffers(2, models);
    glGenBuffers(2, deltaWindDirections);
    if (hasBones)
    {
        glGenBuffers(2, bones);
        glGenBuffers(1, &bonesIndex);
    }
    glGenBuffers(1, &materials);
    gSync[0]     = nullptr;
    gSync[1]     = nullptr;
    ptrModels[0] = nullptr;
    ptrModels[1] = nullptr;
    ptrBones[0]  = nullptr;
    ptrBones[1]  = nullptr;
}

GeometryBatch::~GeometryBatch()
{
    components.clear();
    componentsMap.clear();
    uniqueMeshesMap.clear();
    uniqueMeshesCount.clear();
    bonesCount.clear();

    CleanUp();
    glUseProgram(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indirect);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(2, models);
    glDeleteBuffers(2, deltaWindDirections);
    glDeleteBuffers(2, bones);
    glDeleteBuffers(1, &bonesIndex);
    glDeleteBuffers(1, &materials);
}

void GeometryBatch::CleanUp()
{
    for (int i = 0; i < 2; i++)
    {
        if (gSync[i])
        {
            glDeleteSync(gSync[i]);
            gSync[i] = nullptr;
        }

        if (ptrModels[i])
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, models[i]);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            ptrModels[i] = nullptr;
        }

        if (ptrDeltaWindDirections[i])
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, deltaWindDirections[i]);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            ptrDeltaWindDirections[i] = nullptr;
        }

        if (hasBones && ptrBones[i])
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, bones[i]);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            ptrBones[i] = nullptr;
        }
    }
}

void GeometryBatch::LoadData()
{
    std::vector<Vertex> totalVertices;
    std::vector<unsigned int> totalIndices;
    std::vector<float4x4> totalModels;
    std::vector<MaterialGPU> totalMaterials;

    unsigned int accVertexCount = 0;
    unsigned int accIndexCount  = 0;
    unsigned int accBonesCount  = 0;
    for (const auto& component : components)
    {
        const ResourceMesh* resource = component->GetResourceMesh();

        if (uniqueMeshesMap.find(resource) == uniqueMeshesMap.end())
        {
            const std::vector<Vertex>& vertices      = resource->GetLocalVertices();
            const std::vector<unsigned int>& indices = resource->GetIndices();
            totalVertices.insert(totalVertices.end(), vertices.begin(), vertices.end());
            totalIndices.insert(totalIndices.end(), indices.begin(), indices.end());

            uniqueMeshesMap[resource] = uniqueMeshesMap.size();

            AccMeshCount newMeshCount;
            newMeshCount.accVertexCount = accVertexCount;
            newMeshCount.accIndexCount  = accIndexCount;
            uniqueMeshesCount.push_back(newMeshCount);

            accVertexCount += resource->GetVertexCount();
            accIndexCount  += resource->GetIndexCount();
        }
        componentsMap[component] = componentsMap.size();
        totalModels.push_back(component->GetCombinedMatrix());
        totalMaterials.push_back(component->GetResourceMaterial()->GetMaterial());

        if (hasBones)
        {
            bonesCount.push_back(accBonesCount);
            accBonesCount += static_cast<unsigned int>(component->GetBindMatrices().size());
        }
    }

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, totalVertices.size() * sizeof(Vertex), totalVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, joint));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, totalIndices.size() * sizeof(unsigned int), totalIndices.data(), GL_STATIC_DRAW
    );

    glBindVertexArray(0);

    modelsSize              = totalModels.size() * sizeof(float4x4);
    deltaWindDirectionsSize = totalModels.size() * sizeof(float4);

    const GLbitfield flags  = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT; // const GLbitfield flags  = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT; 
    for (int i = 0; i < 2; i++)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, models[i]);

        glBufferStorage(GL_SHADER_STORAGE_BUFFER, modelsSize, nullptr, flags);
        ptrModels[i] = (float4x4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, modelsSize, flags);

        if (ptrModels[i] == nullptr)
        {
            GLOG("Error mapping ssbo model %d", i);
            return;
        }

        for (size_t j = 0; j < totalModels.size(); ++j)
            ptrModels[i][j] = totalModels[j];

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, deltaWindDirections[i]);

        glBufferStorage(GL_SHADER_STORAGE_BUFFER, deltaWindDirectionsSize, nullptr, flags);
        ptrDeltaWindDirections[i] =
            (float4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, deltaWindDirectionsSize, flags);

        if (ptrDeltaWindDirections[i] == nullptr)
        {
            GLOG("Error mapping delta wind directions %d", i);
            return;
        }

        if (!hasBones) continue;

        bonesSize = accBonesCount * sizeof(float4x4);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bones[i]);

        glBufferStorage(GL_SHADER_STORAGE_BUFFER, bonesSize, nullptr, flags);
        ptrBones[i] = (float4x4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bonesSize, flags);

        if (ptrBones[i] == nullptr)
        {
            GLOG("Error mapping ssbo bones %d", i);
            return;
        }

        for (size_t j = 0; j < accBonesCount; ++j)
            ptrBones[i][j] = float4x4::identity;
    }

    if (hasBones)
    {
        bonesIndexSize = bonesCount.size() * sizeof(unsigned int);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesIndex);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bonesIndexSize, bonesCount.data(), GL_STATIC_DRAW);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER, totalMaterials.size() * sizeof(MaterialGPU), totalMaterials.data(), GL_STATIC_DRAW
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GeometryBatch::Render(const std::vector<MeshComponent*>& meshesToRender, bool shadowMap)
{
#ifdef OPTICK
    OPTICK_PUSH("GeometryBatch::WaitBuffer")
#endif
    WaitBuffer();
#ifdef OPTICK
    OPTICK_POP();
#endif

#ifdef OPTICK
    OPTICK_PUSH("GeometryBatch::Render")
#endif
    std::vector<Command> commands;
    GenerateCommands(meshesToRender, commands);

    if (!updatedOnce) UpdateBuffers(meshesToRender);

    if (!shadowMap)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, materials);
    }
#ifdef OPTICK
    OPTICK_POP()
#endif

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, commands.size() * sizeof(Command), commands.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);

    glMultiDrawElementsIndirect(
        static_cast<GLenum>(mode), GL_UNSIGNED_INT, (GLvoid*)0, static_cast<GLsizei>(commands.size()), 0
    );

    glBindVertexArray(0);
}

void GeometryBatch::GenerateCommands(const std::vector<MeshComponent*>& meshes, std::vector<Command>& commands)
{
    totalVertexCount = 0;
    totalIndexCount  = 0;

    for (MeshComponent* component : meshes)
    {
        if (!component->GetWasEnabled() && !component->GetBatchWasEnabled())
        {
            component->SetBatchWasEnabled();
            continue;
        }

        // CHECK AGAIN BECAUSE IF UPDATE_SHADERSTORAGE == TRUE WILL ARRIVE TO THIS POINT
        if (!component->GetEnabled()) continue;

        const ResourceMesh* resource   = component->GetResourceMesh();

        const unsigned int vertexCount = static_cast<unsigned int>(resource->GetVertexCount());
        const unsigned int indexCount  = static_cast<unsigned int>(resource->GetIndexCount());

        const std::size_t idx          = uniqueMeshesMap[resource];

        Command newCommand;
        newCommand.count          = indexCount;                            // Number of indices in the mesh
        newCommand.instanceCount  = 1;                                     // Number of instances to render
        newCommand.firstIndex     = uniqueMeshesCount[idx].accIndexCount;  // Index offset in the EBO
        newCommand.baseVertex     = uniqueMeshesCount[idx].accVertexCount; // Vertex offset in the VBO
        newCommand.baseInstance   = static_cast<unsigned int>(componentsMap[component]); // Instance Index

        totalVertexCount         += vertexCount;
        totalIndexCount          += indexCount;

        commands.push_back(newCommand);
    }
}

void GeometryBatch::WaitBuffer()
{
    if (gSync[currentBufferIndex])
    {
        GLenum result = glClientWaitSync(gSync[currentBufferIndex], 0, 0);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
        {
            glDeleteSync(gSync[currentBufferIndex]);
            gSync[currentBufferIndex] = nullptr;
        }
        else if (result == GL_TIMEOUT_EXPIRED)
        {
            result = glClientWaitSync(gSync[currentBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); // 1 segundo
            
            if (result == GL_TIMEOUT_EXPIRED)
            {
                glDeleteSync(gSync[currentBufferIndex]);
                gSync[currentBufferIndex] = nullptr;
            }
            else
            {
                glDeleteSync(gSync[currentBufferIndex]);
                gSync[currentBufferIndex] = nullptr;
            }
        }
    }
}

void GeometryBatch::UpdateBuffers(const std::vector<MeshComponent*>& meshesToRender)
{
    updatedOnce               = true;

    const int nextBufferIndex = (currentBufferIndex + 1) % 2;
    const int readBufferIndex  = (currentBufferIndex + 2) % 2;

    if (hasBones)
    {
        const GLuint nextBuffer    = bones[nextBufferIndex];
        const GLuint currentBuffer = bones[readBufferIndex];

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, nextBuffer);

        for (MeshComponent* component : meshesToRender)
        {
            const std::size_t index                         = componentsMap[component];
            const std::size_t accBones                      = bonesCount[index];
            const std::vector<GameObject*>& bonesGameObject = component->GetBonesGO();
            const std::vector<float4x4>& bindMatrices       = component->GetBindMatrices();
            for (size_t i = 0; i < bonesGameObject.size(); ++i)
            {
                ptrBones[nextBufferIndex][accBones + i] = bonesGameObject[i]->GetGlobalTransform() * bindMatrices[i];
            }

            // SETTING BONE INDEX FOR USE IN SHADER SCRIPTS
            component->SetBaseIndex((unsigned int)index);
            component->SetBoneIndexOffset((unsigned int)accBones);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, currentBuffer);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 12, currentBuffer, 0, bonesSize);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesIndex);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, bonesIndex);

        glUniform1i(7, 1); // mesh has bones
    }
    else glUniform1i(7, 0); // meshes has no bones

    const GLuint nextBuffer    = models[nextBufferIndex];
    const GLuint currentBuffer = models[readBufferIndex];

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, nextBuffer);

    for (const MeshComponent* component : meshesToRender)
    {
        const std::size_t index           = componentsMap[component];
        ptrModels[nextBufferIndex][index] = component->GetCombinedMatrix();
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, currentBuffer);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 10, currentBuffer, 0, modelsSize);

    if (const WindConfig* windConfig = App->GetSceneModule()->GetScene()->GetWindsConfig();
        windConfig->GetApplyWindGlobally() && doApplyWind)
    {
        const GLuint nextWindBuffer    = deltaWindDirections[nextBufferIndex];
        const GLuint currentWindBuffer = deltaWindDirections[readBufferIndex];

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, nextWindBuffer);

        Quat windDirection = Quat::FromEulerXYZ(0, windConfig->GetWindDirection() * DEGREE_RAD_CONV, 0);
        for (const MeshComponent* component : meshesToRender)
        {
            Quat deltaWindDirection = windDirection * Quat(component->GetCombinedMatrix().RotatePart().Transposed());
            const std::size_t index = componentsMap[component];
            ptrDeltaWindDirections[nextBufferIndex][index] =
                float4(deltaWindDirection.x, deltaWindDirection.y, deltaWindDirection.z, deltaWindDirection.w);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, currentWindBuffer);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 14, currentWindBuffer, 0, deltaWindDirectionsSize);
    }

    LockBuffer();
}

void GeometryBatch::SwapBuffers()
{
    currentBufferIndex = (currentBufferIndex + 1) % 2;
}

void GeometryBatch::BindBonesBuffer()
{
    const GLuint currentBuffer = bones[currentBufferIndex];

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, currentBuffer);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 12, currentBuffer, 0, bonesSize);
}

void GeometryBatch::UnbindBonesBuffer()
{
    const GLuint currentBuffer = bones[currentBufferIndex];

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, 0);
}

void SOBRASADA_API_ENGINE GeometryBatch::BindMaterialsBuffer()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, materials);
}

void SOBRASADA_API_ENGINE GeometryBatch::UnbindMaterialsBuffer()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, 0);
}

void GeometryBatch::LockBuffer()
{
    const int nextBufferIndex = (currentBufferIndex + 1) % 2;

    if (gSync[nextBufferIndex])
    {
        glDeleteSync(gSync[nextBufferIndex]);
    }
    gSync[nextBufferIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}