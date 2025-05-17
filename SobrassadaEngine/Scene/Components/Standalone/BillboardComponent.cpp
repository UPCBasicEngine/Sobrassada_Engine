#include "BillboardComponent.h"

#include "Application.h"
#include "BillboardModule.h"
#include "ResourceMaterial.h"
#include "EditorUIModule.h"
#include "LibraryModule.h"

#include "glew.h"
#include "imgui.h"

BillboardComponent::BillboardComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Billboard", COMPONENT_BILLBOARD)
{
}

BillboardComponent::BillboardComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Tag")) billboardTag = HashString(initialState["Tag"].GetString());
    if (initialState.HasMember("Material")) currentMaterialUID = initialState["Material"].GetUint64();

    if (initialState.HasMember("Height")) height = initialState["Height"].GetFloat();
    if (initialState.HasMember("Width")) width = initialState["Width"].GetFloat();

    if (initialState.HasMember("XTiles")) xTiles = initialState["XTiles"].GetInt();
    if (initialState.HasMember("YTiles")) yTiles = initialState["YTiles"].GetInt();
    if (initialState.HasMember("SpriteSpeed")) spriteSpeed = initialState["SpriteSpeed"].GetFloat();

    if (billboardTag.GetString() != "") App->GetBillboardModule()->RequestTag(billboardTag, this);
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
    targetState.AddMember("Tag", rapidjson::Value(billboardTag.GetString().c_str(), allocator), allocator);

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

    if (ImGui::Checkbox("Lock Pitch", &lockPitch)) App->GetBillboardModule()->UpdateTagLockPitch(billboardTag, lockPitch);

    if (ImGui::InputFloat("Width", &width)) App->GetBillboardModule()->UpdateTagWidth(billboardTag, width);
    if (ImGui::InputFloat("Height", &height)) App->GetBillboardModule()->UpdateTagHeight(billboardTag, height);

    ImGui::InputInt("Texture X tiles", &xTiles);
    ImGui::InputInt("Texture Y tiles", &yTiles);
    ImGui::InputFloat("Animation speed", &spriteSpeed);

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

void BillboardComponent::ParentUpdated()
{
    App->GetBillboardModule()->UpdateTagPositions(billboardTag);
}

void BillboardComponent::ClearBillboardData()
{
    currentMaterial     = nullptr;
    currentMaterialName = "No material";
    currentMaterialUID  = INVALID_UID;
    billboardTag        = HashString("");

    width               = 1.f;
    height              = 1.f;
    lockPitch           = false;

    xTiles              = 0;
    yTiles              = 0;
    spriteSpeed         = 0;
}

void BillboardComponent::SetMaterial(ResourceMaterial* newMaterial)
{
    currentMaterial     = newMaterial;
    currentMaterialName = currentMaterial->GetName();
    currentMaterialUID  = currentMaterial->GetUID();
}
