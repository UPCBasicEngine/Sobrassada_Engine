#include "Component.h"

#include "GameObject.h"

#include "Math/float4x4.h"
#include "imgui.h"
#include <string>

Component::Component(UID uid, GameObject* parent, const char* initName, ComponentType type)
    : uid(uid), parent(parent), type(type), enabled(true)
{
    memcpy(name, initName, strlen(initName));

    localComponentAABB.SetNegativeInfinity();
}

Component::Component(const rapidjson::Value& initialState, GameObject* parent)
    : uid(initialState["UID"].GetUint64()), parent(parent),
      type(static_cast<ComponentType>(initialState["Type"].GetInt()))
{
    enabled              = initialState["Enabled"].GetBool();
    if (initialState.HasMember("WasEnabled"))
    {
        wasEnabled = initialState["WasEnabled"].GetBool();
    }

    const char* initName = initialState["Name"].GetString();
    memcpy(name, initName, strlen(initName));

    localComponentAABB.SetNegativeInfinity();
}

void Component::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    targetState.AddMember("UID", uid, allocator);
    targetState.AddMember("Type", type, allocator);

    targetState.AddMember("Enabled", enabled, allocator);
    targetState.AddMember("WasEnabled", wasEnabled, allocator);
    targetState.AddMember("Name", rapidjson::Value(std::string(name).c_str(), allocator), allocator);
}

void Component::RenderEditorInspector()
{
    ImGui::InputText("Name", name, sizeof(name));
    ImGui::SameLine();

    if (parent->IsGloballyEnabled())
    {
        if (ImGui::Checkbox("Enabled", &enabled)) wasEnabled = enabled;
        enabled = wasEnabled;
    }
    else
    {
        ImGui::Checkbox("Enabled", &enabled);
        enabled = false;
    }
}

UID Component::GetParentUID() const
{
    return parent->GetUID();
}

const float4x4& Component::GetGlobalTransform() const
{
    return parent->GetGlobalTransform();
}

bool Component::IsEffectivelyEnabled() const
{
    return enabled && parent->IsGloballyEnabled();
}
