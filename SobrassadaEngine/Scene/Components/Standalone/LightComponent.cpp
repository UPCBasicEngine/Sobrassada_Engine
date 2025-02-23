#include "LightComponent.h"

#include "LightsConfig.h"

#include "ImGui.h"

LightComponent::LightComponent(
    const UID uid, const UID uidParent, const UID uidRoot, const char* uiName, const ComponentType lightType,
    const Transform& parentGlobalTransform
)
    : Component(uid, uidParent, uidRoot, uiName, lightType, parentGlobalTransform)
{
    intensity  = 1;
    color      = float3(1.0f, 1.0f, 1.0f);
    drawGizmos = true;
}

LightComponent::LightComponent(const rapidjson::Value& initialState) : Component(initialState)
{
    if (initialState.HasMember("Intensity"))
    {
        intensity = initialState["Intensity"].GetFloat();
    }
    if (initialState.HasMember("ColorRed"))
    {
        color.x = initialState["ColorRed"].GetFloat();
    }
    if (initialState.HasMember("ColorGreen"))
    {
        color.y = initialState["ColorGreen"].GetFloat();
    }
    if (initialState.HasMember("ColorBlue"))
    {
        color.z = initialState["ColorBlue"].GetFloat();
    }
    if (initialState.HasMember("DrawGizmos"))
    {
        drawGizmos = initialState["DrawGizmos"].GetBool();
    }
}

void LightComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("Intensity", intensity, allocator);
    targetState.AddMember("ColorRed", color.x, allocator);
    targetState.AddMember("ColorGreen", color.y, allocator);
    targetState.AddMember("ColorBlue", color.z, allocator);
    targetState.AddMember("DrawGizmos", drawGizmos, allocator);
}

void LightComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (enabled)
    {
        ImGui::SeparatorText("Light");
        ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
        ImGui::Checkbox("Draw gizmos", &drawGizmos);
        ImGui::Spacing();
    }
}

void LightComponent::Update()
{
}

void LightComponent::Render()
{
}

