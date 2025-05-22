#include "DecalComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "Modules/ResourcesModule.h"
#include "ResourceMaterial.h"
#include "ResourceTexture.h"

#include "imgui.h"

DecalComponent::DecalComponent(UID uid, GameObject* parent) : Component(uid, parent, "Decal", COMPONENT_BILLBOARD)
{
    RecalculateAABB();
}

DecalComponent::DecalComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Material")) currentMaterialUID = initialState["Material"].GetUint64();
    if (initialState.HasMember("Height")) height = initialState["Height"].GetFloat();
    if (initialState.HasMember("Width")) width = initialState["Width"].GetFloat();

    RecalculateAABB();
}

DecalComponent::~DecalComponent()
{
}

void DecalComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember(
        "Material", currentMaterial != nullptr ? currentMaterial->GetUID() : DEFAULT_MATERIAL_UID, allocator
    );

    targetState.AddMember("Height", height, allocator);
    targetState.AddMember("Width", width, allocator);
}

void DecalComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_BILLBOARD)
    {
        const DecalComponent* otherDecal = static_cast<const DecalComponent*>(other);
        enabled                          = otherDecal->enabled;
        wasEnabled                       = otherDecal->wasEnabled;

        width                            = otherDecal->width;
        height                           = otherDecal->height;
    }
}

void DecalComponent::Update(float deltaTime)
{
}

void DecalComponent::Render(float deltaTime)
{
}

void DecalComponent::RenderDebug(float deltaTime)
{
}

void DecalComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Decal Component");

    ImGui::Separator();

    if (ImGui::InputFloat("Width", &width)) RecalculateAABB();
    if (ImGui::InputFloat("Height", &height)) RecalculateAABB();

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

void DecalComponent::AddMaterial(UID resource)
{
    if (resource == INVALID_UID || App->GetResourcesModule()->RequestResource(resource) == nullptr)
    {
        resource = DEFAULT_MATERIAL_UID;
    }

    if (currentMaterial != nullptr && currentMaterial->GetUID() == resource) return;

    ResourceMaterial* newMaterial =
        dynamic_cast<ResourceMaterial*>(App->GetResourcesModule()->RequestResource(resource));

    if (!newMaterial) return;
    currentMaterial     = newMaterial;
    currentResourceName = currentMaterial->GetName();
    currentMaterialUID  = currentMaterial->GetUID();
}

void DecalComponent::ParentUpdated()
{
    RecalculateAABB();
}

void DecalComponent::RecalculateAABB()
{
    float3 localPosition  = parent->GetLocalTransform().TranslatePart();
    float maxValue        = width > height ? width : height;
    maxValue             /= 2.f;
    localComponentAABB    = AABB(float3(-maxValue, -maxValue, -maxValue), float3(maxValue, maxValue, maxValue));

    parent->OnAABBUpdated();
}