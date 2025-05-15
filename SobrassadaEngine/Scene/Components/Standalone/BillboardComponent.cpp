#include "BillboardComponent.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "ResourceMaterial.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "ShaderModule.h"

#include "glew.h"
#include "imgui.h"

BillboardComponent::BillboardComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Billboard", COMPONENT_BILLBOARD)
{
    CreateVertexBufferObject();
}

BillboardComponent::BillboardComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Material"))
    {
        UID materialUID = initialState["Material"].GetUint64();
        if (materialUID != INVALID_UID) AddMaterial(materialUID);
    }

    if (initialState.HasMember("Height")) height = initialState["Height"].GetFloat();
    if (initialState.HasMember("Width")) width = initialState["Width"].GetFloat();

    if (initialState.HasMember("XTiles")) xTiles = initialState["XTiles"].GetInt();
    if (initialState.HasMember("YTiles")) yTiles = initialState["YTiles"].GetInt();
    if (initialState.HasMember("SpriteSpeed")) spriteSpeed = initialState["SpriteSpeed"].GetFloat();

    CreateVertexBufferObject();
}

BillboardComponent::~BillboardComponent()
{
    if (currentMaterial) App->GetResourcesModule()->ReleaseResource(currentMaterial);
    glDeleteBuffers(1, &vbo);
}

void BillboardComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("Material", currentMaterial != nullptr ? currentMaterial->GetUID() : INVALID_UID, allocator);
    targetState.AddMember("Height", height, allocator);
    targetState.AddMember("Width", width, allocator);

    targetState.AddMember("XTiles", xTiles, allocator);
    targetState.AddMember("YTiles", yTiles, allocator);
    targetState.AddMember("SpriteSpeed", spriteSpeed, allocator);
}

void BillboardComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_BILLBOARD)
    {
        const BillboardComponent* otherBillboard = static_cast<const BillboardComponent*>(other);
        enabled                                  = otherBillboard->enabled;
        wasEnabled                               = otherBillboard->wasEnabled;
    }
}

void BillboardComponent::Update(float deltaTime)
{
    CameraComponent* activeCamera = App->GetSceneModule()->GetScene()->GetMainCamera();
    if (activeCamera && App->GetSceneModule()->GetInPlayMode())
    {
    }
    else
    {
        const Frustum& editorCamera = App->GetCameraModule()->GetCamera();

        float3 frontVector          = editorCamera.pos - parent->GetPosition();
        frontVector.Normalize();

        float3x3 rotationMatrix =
            float3x3(editorCamera.WorldRight(), lockPitch ? float3(0, 1.f, 0) : editorCamera.up, frontVector);

        const float4x4& originalTransform = parent->GetLocalTransform();
        float4x4 newLocalTransform =
            float4x4::FromTRS(originalTransform.TranslatePart(), rotationMatrix, originalTransform.GetScale());

        parent->SetLocalTransform(newLocalTransform);
    }
}

void BillboardComponent::Render(float deltaTime)
{
    if (currentMaterial && vbo)
    {
        float4x4 model, view, proj;

        model = parent->GetGlobalTransform();
        view  = App->GetCameraModule()->GetViewMatrix();
        proj  = App->GetCameraModule()->GetProjectionMatrix();

        glUseProgram(App->GetShaderModule()->GetBillboardProgram());
        glUniformMatrix4fv(0, 1, GL_TRUE, &proj[0][0]);
        glUniformMatrix4fv(1, 1, GL_TRUE, &view[0][0]);
        glUniformMatrix4fv(2, 1, GL_TRUE, &model[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Sending texture coordiantes
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 6));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentMaterial->GetDiffuseColorID());

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void BillboardComponent::RenderDebug(float deltaTime)
{
}

void BillboardComponent::RenderEditorInspector()
{
    ImGui::Checkbox("Lock Pitch", &lockPitch);

    if (ImGui::InputFloat("Width", &width)) CreateVertexBufferObject();
    if (ImGui::InputFloat("Height", &height)) CreateVertexBufferObject();

    ImGui::InputInt("Texture X tiles", &xTiles);
    ImGui::InputInt("Texture Y tiles", &yTiles);
    ImGui::InputFloat("Animation speed", &spriteSpeed);

    ImGui::SeparatorText("Material");
    ImGui::Text(currentMaterialName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Select material"))
    {
        ImGui::OpenPopup(CONSTANT_MATERIAL_SELECT_DIALOG_ID);
    }

    if (ImGui::IsPopupOpen(CONSTANT_MATERIAL_SELECT_DIALOG_ID))
    {

        const UID chosenMatUID = App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
            CONSTANT_MATERIAL_SELECT_DIALOG_ID, App->GetLibraryModule()->GetMaterialMap(), INVALID_UID
        );

        if (chosenMatUID != INVALID_UID) AddMaterial(chosenMatUID);
    }

    if (currentMaterial != nullptr) currentMaterial->OnEditorUpdate();
}

void BillboardComponent::CreateVertexBufferObject()
{
    // vertices -> texture coords

    float vertexData[] = {
        -width / 2.f, height / 2.f,  0.f, //
        -width / 2.f, -height / 2.f, 0.f, //
        width / 2.f,  -height / 2.f, 0.f, //

        -width / 2.f, height / 2.f,  0.f, //
        width / 2.f,  -height / 2.f, 0.f, //
        width / 2.f,  height / 2.f,  0.f, //

        0.f,          1.f, //
        0.f,          0.f, //
        1.f,          0.f, //

        0.f,          1.f, //
        1.f,          0.f, //
        1.f,          1.f, //
    };

    if (vbo == 0) glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    localComponentAABB = AABB(float3(-width / 2.f, -height / 2.f, 0.f), float3(width / 2.f, height / 2.f, 0.f));
}

void BillboardComponent::AddMaterial(UID resourceUID)
{
    if (resourceUID == INVALID_UID || App->GetResourcesModule()->RequestResource(resourceUID) == nullptr)
    {
        resourceUID = DEFAULT_MATERIAL_UID;
    }

    if (currentMaterial != nullptr && currentMaterial->GetUID() == resourceUID) return;

    ResourceMaterial* newMaterial =
        dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(resourceUID));
    if (newMaterial != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentMaterial);
        currentMaterial     = newMaterial;
        currentMaterialName = currentMaterial->GetName();
    }
}
