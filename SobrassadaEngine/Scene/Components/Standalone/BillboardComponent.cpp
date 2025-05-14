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

#include "glew.h"
#include "imgui.h"

BillboardComponent::BillboardComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Billboard", COMPONENT_BILLBOARD)
{
}

BillboardComponent::BillboardComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

BillboardComponent::~BillboardComponent()
{
    glDeleteBuffers(1, &vbo);
}

void BillboardComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
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

        float3x3 rotationMatrix           = float3x3(editorCamera.WorldRight(), editorCamera.up, frontVector);

        const float4x4& originalTransform = parent->GetLocalTransform();
        float4x4 newLocalTransform =
            float4x4::FromTRS(originalTransform.TranslatePart(), rotationMatrix, originalTransform.GetScale());

        parent->SetLocalTransform(newLocalTransform);
    }
}

void BillboardComponent::Render(float deltaTime)
{
}

void BillboardComponent::RenderDebug(float deltaTime)
{
}

void BillboardComponent::RenderEditorInspector()
{
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
    unsigned int numVertices = 6;

    // vertices -> texture coords -> normals
    float vertexData[]       = {
        -1.f, 1.f,  0.f, //
        -1.f, -1.f, 0.f, //
        1.f,  -1.f, 0.f, //

        -1.f, 1.f,  0.f, //
        1.f,  -1.f, 0.f, //
        1.f,  1.f,  0.f, //

        0.f,  1.f, //
        0.f,  0.f, //
        1.f,  0.f, //

        0.f,  1.f, //
        1.f,  0.f, //
        1.f,  1.f, //

        0.f,  0.f,  1.f, //
        0.f,  0.f,  1.f, //
        0.f,  0.f,  1.f, //

        0.f,  0.f,  1.f, //
        0.f,  0.f,  1.f, //
        0.f,  0.f,  1.f, //
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
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
