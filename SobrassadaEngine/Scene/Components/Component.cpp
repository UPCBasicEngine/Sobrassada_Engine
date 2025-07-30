#include "Component.h"

#include "GameObject.h"

#include "Math/float4x4.h"
#include "imgui.h"
#include <string>

#ifdef OPTICK
#include "optick.h"
#endif

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
    enabled = initialState["Enabled"].GetBool();
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
    targetState.AddMember("Name", rapidjson::Value(name, allocator), allocator);
}

bool Component::RenderGizmo()
{
    return false;
}

void Component::RenderEditorInspector()
{
    ImGui::InputText("Name", name, sizeof(name));
    ImGui::SameLine();

    if (parent->IsGloballyEnabled())
    {
        bool previousEnabled = enabled;
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            wasEnabled = previousEnabled;
        }
    }
    else
    {
        ImGui::Checkbox("Enabled", &enabled);
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
    return enabled && parent && parent->IsGloballyEnabled();
}
