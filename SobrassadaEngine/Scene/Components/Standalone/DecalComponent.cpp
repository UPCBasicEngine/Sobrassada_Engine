#include "DecalComponent.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "LibraryModule.h"
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
    if (initialState.HasMember("Texture")) currentTextureUID = initialState["Texture"].GetUint64();
    if (initialState.HasMember("UseTexture")) useTexture = initialState["UseTexture"].GetBool();
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

    targetState.AddMember(
        "Texture", currentTexture != nullptr ? currentTexture->GetUID() : FALLBACK_TEXTURE_UID, allocator
    );

    targetState.AddMember("Height", height, allocator);
    targetState.AddMember("Width", width, allocator);
    targetState.AddMember("UseTexture", useTexture, allocator);
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
        useTexture                       = otherDecal->useTexture;
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

    if (ImGui::BeginCombo("Resource type", DecalResourceTypeStrings[useTexture ? 1 : 0]))
    {
        for (int i = 0; i < DecalResourceTypeStringsSize; ++i)
        {
            if (ImGui::Selectable(DecalResourceTypeStrings[i])) useTexture = i;
        }
        ImGui::EndCombo();
    }

    if (!useTexture)
    {
        if (ImGui::Button("Select material"))
        {
            ImGui::OpenPopup(CONSTANT_MATERIAL_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_MATERIAL_SELECT_DIALOG_ID))
        {
            const UID chosenMatUID = App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                CONSTANT_MATERIAL_SELECT_DIALOG_ID, App->GetLibraryModule()->GetMaterialMap(), INVALID_UID
            );
        }

        if (currentMaterial != nullptr) currentMaterial->OnEditorUpdate();
    }
    else
    {
        if (ImGui::Button("Select texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {

            const UID chosenTexUID = App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
            );
        }

        if (currentTexture != nullptr)
        {
            ImGui::Text("Diffuse Texture");
            ImGui::Image((ImTextureID)(intptr_t)currentTexture->GetTextureID(), ImVec2(256, 256));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(
                    "Texture Dimensions: %d, %d", currentTexture->GetTextureWidth(), currentTexture->GetTextureWidth()
                );
            }
        }
    }
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