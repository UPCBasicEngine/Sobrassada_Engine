#include "BillboardComponent.h"

#include "Application.h"
#include "ResourceMaterial.h"
#include "BillboardModule.h"

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

    auto& billboardTags = App->GetBillboardModule()->GetTags();

    if (ImGui::BeginCombo("Billboard tag", billboardTag.GetString().c_str()))
    {
        for (int i = 0; i < billboardTags.size(); ++i)
        {
            if (ImGui::Selectable(billboardTags[i].GetString().c_str()))
            {
                billboardTag = billboardTags[i];
                App->GetBillboardModule()->RequestTag(billboardTag, this);
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::Button("Delete current tag"))
    {
        App->GetBillboardModule()->DeleteTag(billboardTag);
        billboardTag = HashString("");
    }
}
