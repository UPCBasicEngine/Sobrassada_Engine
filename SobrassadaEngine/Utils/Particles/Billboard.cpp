#include "Billboard.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "GameObject.h"
#include "OpenGLModule.h"
#include "ResourceMaterial.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "ShaderModule.h"
#include "Standalone/BillboardComponent.h"

#include "Math/float2.h"
#include "glew.h"
#include <algorithm>
#include <chrono>

Billboard::Billboard(float width, float height) : width(width), height(height)
{
    CreateVertexBufferObject();
}

Billboard::~Billboard()
{
    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->ClearBillboardData();
    }

    if (material) App->GetResourcesModule()->ReleaseResource(material);
    if (texture) App->GetResourcesModule()->ReleaseResource(texture);
    glDeleteBuffers(1, &vbo);
}

void Billboard::UpdateWidth(float newWidth)
{
    width = newWidth;

    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->SetWidth(width);
    }
}

void Billboard::UpdateHeight(float newHeight)
{
    height = newHeight;

    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->SetHeight(height);
    }
}

void Billboard::UpdateMaterial(UID newMaterialUID)
{
    if (newMaterialUID == INVALID_UID || App->GetResourcesModule()->RequestResource(newMaterialUID) == nullptr)
    {
        newMaterialUID = DEFAULT_MATERIAL_UID;
    }

    if (material != nullptr && material->GetUID() == newMaterialUID) return;

    ResourceMaterial* newMaterial =
        dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(newMaterialUID));

    if (newMaterial != nullptr)
    {
        useTexture = false;

        App->GetResourcesModule()->ReleaseResource(material);
        material = newMaterial;

        for (auto billboardComponent : instanceComponents)
        {
            billboardComponent->SetMaterial(material);
        }
    }
}

void Billboard::UpdateTexture(UID newTextureUID)
{
    if (newTextureUID == INVALID_UID || App->GetResourcesModule()->RequestResource(newTextureUID) == nullptr)
    {
        newTextureUID = FALLBACK_TEXTURE_UID;
    }

    if (texture != nullptr && texture->GetUID() == newTextureUID) return;

    ResourceTexture* newTexture =
        dynamic_cast<ResourceTexture*>(App->GetResourcesModule()->RequestResource(newTextureUID));

    if (newTexture != nullptr)
    {
        useTexture = true;

        App->GetResourcesModule()->ReleaseResource(texture);
        texture = newTexture;

        for (auto billboardComponent : instanceComponents)
        {
            billboardComponent->SetTexture(texture);
        }
    }
}

void Billboard::UpdateLockPitch(bool newLock)
{
    lockPitch = newLock;

    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->SetLockPitch(lockPitch);
    }
}

void Billboard::UpdateUseTexture(bool newTexture)
{
    useTexture = newTexture;

    for (auto billboardComponent : instanceComponents)
    {
        billboardComponent->SetUseTexture(useTexture);
    }
}

void Billboard::Render(const float4x4& VP, const float3& rightVector, const float3& upVector)
{
    if ((useTexture ? texture != nullptr : material != nullptr) && vbo && positionsVbo)
    {
        const auto start        = std::chrono::high_resolution_clock::now();

        float4x4 viewProjection = VP;
        float3 cameraRight      = rightVector;
        float3 cameraUp         = lockPitch ? float3(0, 1, 0) : upVector;
        float2 billboardSize    = float2(width, height);

        glUseProgram(App->GetShaderModule()->GetBillboardProgram());
        glUniform3fv(0, 1, &cameraRight[0]);
        glUniform3fv(1, 1, &cameraUp[0]);
        glUniform2fv(2, 1, &billboardSize[0]);
        glUniformMatrix4fv(3, 1, GL_TRUE, &viewProjection[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Sending vertex coords
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Sending texture coordiantes
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 6));

        // Sending center billboard positions
        glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribDivisor(2, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, useTexture ? texture->GetTextureID() : material->GetDiffuseColorID());

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)instanceComponents.size());

        glBindTexture(GL_TEXTURE_2D, 0);

        const auto end                             = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = end - start;

        unsigned int totalTrangles                 = (unsigned int)instancePositions.size() * 2;

        App->GetOpenGLModule()->AddTrianglesPerSecond(totalTrangles / elapsed.count());
        App->GetOpenGLModule()->AddVerticesCount(totalTrangles * 3);
        App->GetOpenGLModule()->AddDrawCallsCount();
    }
}

void Billboard::CreateVertexBufferObject()
{
    // vertices -> texture coords

    float vertexData[] = {
        -0.5, 0.5,  0.f, //
        -0.5, -0.5, 0.f, //
        0.5,  -0.5, 0.f, //

        -0.5, 0.5,  0.f, //
        0.5,  -0.5, 0.f, //
        0.5,  0.5,  0.f, //

        0.f,  1.f, //
        0.f,  0.f, //
        1.f,  0.f, //

        0.f,  1.f, //
        1.f,  0.f, //
        1.f,  1.f, //
    };

    if (vbo == 0) glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    if (positionsVbo == 0) glGenBuffers(1, &positionsVbo);
}

void Billboard::UpdatePositionsVbo(const float3& cameraPosition)
{
    std::sort(
        instancePositions.begin(), instancePositions.end(),
        [cameraPosition](const float3& a, const float3& b)
        {
            float distanceA = (a - cameraPosition).LengthSq();
            float distanceB = (b - cameraPosition).LengthSq();

            return distanceA > distanceB;
        }
    );

    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * instancePositions.size(), &instancePositions[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Billboard::AddComponent(BillboardComponent* newBillboard)
{
    bool playMode                     = App->GetSceneModule()->GetInPlayMode();
    const Frustum& editorCamera       = App->GetCameraModule()->GetCamera();
    const CameraComponent* gameCamera = App->GetSceneModule()->GetScene()->GetMainCamera();

    float3 cameraPosition =
        playMode ? gameCamera ? gameCamera->GetCameraPosition() : editorCamera.pos : editorCamera.pos;

    auto iterator = instanceComponents.insert(instanceComponents.end(), newBillboard);

    newBillboard->SetWidth(width);
    newBillboard->SetHeight(height);
    if (useTexture) newBillboard->SetTexture(texture);
    else newBillboard->SetMaterial(material);
    newBillboard->SetIterator(iterator);

    instancePositions.push_back(newBillboard->GetParent()->GetGlobalTransform().TranslatePart());
    UpdatePositionsVbo(cameraPosition);
}

void Billboard::RemoveComponent(std::list<BillboardComponent*>::iterator billboardIterator)
{
    instanceComponents.erase(billboardIterator);
    reloadPositions = true;
}

void Billboard::CheckReloadPositions()
{
    if (reloadPositions)
    {
        reloadPositions = false;

        instancePositions.clear();

        for (auto component : instanceComponents)
        {
            instancePositions.push_back(component->GetParent()->GetGlobalTransform().TranslatePart());
        }

        bool playMode                     = App->GetSceneModule()->GetInPlayMode();
        const Frustum& editorCamera       = App->GetCameraModule()->GetCamera();
        const CameraComponent* gameCamera = App->GetSceneModule()->GetScene()->GetMainCamera();

        float3 cameraPosition =
            playMode ? gameCamera ? gameCamera->GetCameraPosition() : editorCamera.pos : editorCamera.pos;

        UpdatePositionsVbo(cameraPosition);
    }
}
