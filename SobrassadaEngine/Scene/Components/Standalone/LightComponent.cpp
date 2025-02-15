#include "LightComponent.h"

#include "LightsConfig.h"

#include "ImGui.h"

LightComponent::LightComponent(
    const UID uid, const UID uidParent, const UID uidRoot, const char *uiName, const ComponentType lightType,
    const Transform &parentGlobalTransform
)
    : Component(uid, uidParent, uidRoot, uiName, type, parentGlobalTransform)
{
    intensity  = 1;
    color      = float3(1.0f, 1.0f, 1.0f);
    drawGizmos = true;
}

void LightComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (enabled)
    {
        ImGui::SeparatorText("Light");
    }
}

void LightComponent::Update() {}

void LightComponent::Render() {}
