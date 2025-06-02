#include "BillboardComponent.h"

#include "Application.h"
#include "BillboardModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "LibraryModule.h"
#include "ResourceMaterial.h"
#include "ResourceTexture.h"

#include "glew.h"
#include "imgui.h"

BillboardComponent::BillboardComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Billboard", COMPONENT_BILLBOARD)
{
    RecalculateAABB();
}

BillboardComponent::BillboardComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Tag")) billboardTag = HashString(initialState["Tag"].GetString());
    if (initialState.HasMember("Material")) currentMaterialUID = initialState["Material"].GetUint64();
    if (initialState.HasMember("Texture")) currentTextureUID = initialState["Texture"].GetUint64();

    if (initialState.HasMember("UseTexture")) useTexture = initialState["UseTexture"].GetBool();
    if (initialState.HasMember("LockPitch")) lockPitch = initialState["LockPitch"].GetBool();

    if (initialState.HasMember("Height")) height = initialState["Height"].GetFloat();
    if (initialState.HasMember("Width")) width = initialState["Width"].GetFloat();

    if (billboardTag.GetString() != "")
    {
        App->GetBillboardModule()->RequestTag(billboardTag, this);

        if (initialState.HasMember("Xmin")) xmin = initialState["Xmin"].GetFloat();
        if (initialState.HasMember("Ymin")) ymin = initialState["Ymin"].GetFloat();
        if (initialState.HasMember("SelectionWidth")) selectionWidth = initialState["SelectionWidth"].GetFloat();
        if (initialState.HasMember("SelectionHeight")) selectionHeight = initialState["SelectionHeight"].GetFloat();

        App->GetBillboardModule()->UpdateTagUVCoords(billboardTag, xmin, ymin, selectionWidth, selectionHeight);

        RecalculateAABB();
    }
}

BillboardComponent::~BillboardComponent()
{
    App->GetBillboardModule()->RemoveComponentFromTag(billboardTag, this);
}

void BillboardComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember(
        "Material", currentMaterial != nullptr ? currentMaterial->GetUID() : DEFAULT_MATERIAL_UID, allocator
    );

    targetState.AddMember(
        "Texture", currentTexture != nullptr ? currentTexture->GetUID() : FALLBACK_TEXTURE_UID, allocator
    );

    targetState.AddMember("Tag", rapidjson::Value(billboardTag.GetString().c_str(), allocator), allocator);

    targetState.AddMember("Height", height, allocator);
    targetState.AddMember("Width", width, allocator);
    targetState.AddMember("UseTexture", useTexture, allocator);
    targetState.AddMember("LockPitch", lockPitch, allocator);

    targetState.AddMember("Xmin", xmin, allocator);
    targetState.AddMember("Ymin", ymin, allocator);
    targetState.AddMember("SelectionWidth", selectionWidth, allocator);
    targetState.AddMember("SelectionHeight", selectionHeight, allocator);
}

void BillboardComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_BILLBOARD)
    {
        const BillboardComponent* otherBillboard = static_cast<const BillboardComponent*>(other);
        enabled                                  = otherBillboard->enabled;
        wasEnabled                               = otherBillboard->wasEnabled;

        billboardTag                             = otherBillboard->billboardTag;

        App->GetBillboardModule()->RequestTag(billboardTag, this);
    }
}

void BillboardComponent::Update(float deltaTime)
{
}

void BillboardComponent::Render(float deltaTime)
{
}

void BillboardComponent::RenderDebug(float deltaTime)
{
}

void BillboardComponent::RenderEditorInspector()
{
    ImGui::InputText("New tag name", newTagName, IM_ARRAYSIZE(newTagName));

    if (ImGui::Button("Create tag"))
    {
        App->GetBillboardModule()->CreateTag(newTagName);
        memset(newTagName, 0, sizeof(newTagName));
    }

    ImGui::Text("Current tag: %s", billboardTag.GetString().c_str());
    ImGui::SameLine();
    if (ImGui::Button("Change tag"))
    {
        ImGui::OpenPopup("BillboardSelection");
    }

    if (ImGui::BeginPopup("BillboardSelection"))
    {
        auto& billboardTags = App->GetBillboardModule()->GetTags();

        if (ImGui::BeginListBox(
                "##BillboardSelectionList",
                ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeightWithSpacing() * billboardTags.size())
            ))
        {

            for (int i = 0; i < billboardTags.size(); ++i)
            {
                if (ImGui::Selectable(billboardTags[i].GetString().c_str()))
                {
                    if (billboardTags[i] != billboardTag)
                    {
                        HashString selectedTag = billboardTags[i];
                        App->GetBillboardModule()->RequestTag(selectedTag, this);
                        billboardTag = std::move(selectedTag);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::EndPopup();
    }

    if (ImGui::Button("Delete current tag")) App->GetBillboardModule()->DeleteTag(billboardTag);

    ImGui::Separator();

    if (ImGui::Checkbox("Lock Pitch", &lockPitch))
        App->GetBillboardModule()->UpdateTagLockPitch(billboardTag, lockPitch);

    if (ImGui::InputFloat("Width", &width)) App->GetBillboardModule()->UpdateTagWidth(billboardTag, width);
    if (ImGui::InputFloat("Height", &height)) App->GetBillboardModule()->UpdateTagHeight(billboardTag, height);

    ImGui::PushItemWidth(100);

    if (ImGui::InputFloat("##xmin", &xmin))
        App->GetBillboardModule()->UpdateTagUVCoords(billboardTag, xmin, ymin, selectionWidth, selectionHeight);
    ImGui::SameLine();
    if (ImGui::InputFloat("##ymin", &ymin))
        App->GetBillboardModule()->UpdateTagUVCoords(billboardTag, xmin, ymin, selectionWidth, selectionHeight);
    ImGui::SameLine();
    ImGui::Text("Starting x,y position");

    if (ImGui::InputFloat("##selectionWidth", &selectionWidth))
        App->GetBillboardModule()->UpdateTagUVCoords(billboardTag, xmin, ymin, selectionWidth, selectionHeight);
    ImGui::SameLine();
    if (ImGui::InputFloat("##selectionHeight", &selectionHeight))
        App->GetBillboardModule()->UpdateTagUVCoords(billboardTag, xmin, ymin, selectionWidth, selectionHeight);
    ImGui::SameLine();
    ImGui::Text("Width and Height");

    ImGui::PopItemWidth();

    ImGui::Separator();

    if (ImGui::BeginCombo("Resource type", ResourceTypeStrings[useTexture ? 1 : 0]))
    {
        for (int i = 0; i < ResourceTypeStringsSize; ++i)
        {
            if (ImGui::Selectable(ResourceTypeStrings[i]))
            {
                useTexture = i;
                App->GetBillboardModule()->UpdateTagUseTexture(billboardTag, useTexture);
            }
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

            if (chosenMatUID != INVALID_UID) App->GetBillboardModule()->UpdateTagMaterial(billboardTag, chosenMatUID);
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

            if (chosenTexUID != INVALID_UID) App->GetBillboardModule()->UpdateTagTexture(billboardTag, chosenTexUID);
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

void BillboardComponent::ParentUpdated()
{
    App->GetBillboardModule()->UpdateTagPositions(billboardTag);
    RecalculateAABB();
}

void BillboardComponent::ClearBillboardData()
{
    currentResourceName = "No material";

    currentMaterial     = nullptr;
    currentMaterialUID  = INVALID_UID;

    currentTexture      = nullptr;
    currentTextureUID   = INVALID_UID;

    billboardTag        = HashString("");

    width               = 1.f;
    height              = 1.f;
    lockPitch           = false;
}

void BillboardComponent::SetWidth(float newWidth)
{
    width = newWidth;
    RecalculateAABB();
}

void BillboardComponent::SetHeight(float newHeight)
{
    height = newHeight;
    RecalculateAABB();
}

void BillboardComponent::SetMaterial(ResourceMaterial* newMaterial)
{
    if (!newMaterial) return;
    useTexture          = false;
    currentMaterial     = newMaterial;
    currentResourceName = currentMaterial->GetName();
    currentMaterialUID  = currentMaterial->GetUID();

    xmin                = 0.f;
    ymin                = 0.f;
    selectionWidth      = (float)currentMaterial->GetDiffuseWidth();
    selectionHeight     = (float)currentMaterial->GetDiffuseHeight();
}

void BillboardComponent::SetTexture(ResourceTexture* newTexture)
{
    if (!newTexture) return;
    useTexture          = true;
    currentTexture      = newTexture;
    currentResourceName = currentTexture->GetName();
    currentMaterialUID  = currentTexture->GetUID();

    xmin                = 0.f;
    ymin                = 0.f;
    selectionWidth      = (float)currentTexture->GetTextureWidth();
    selectionHeight     = (float)currentTexture->GetTextureHeight();
}

void BillboardComponent::RecalculateAABB()
{
    float3 localPosition  = parent->GetLocalTransform().TranslatePart();
    float maxValue        = width > height ? width : height;
    maxValue             /= 2.f;
    localComponentAABB    = AABB(float3(-maxValue, -maxValue, -maxValue), float3(maxValue, maxValue, maxValue));

    parent->OnAABBUpdated();
}