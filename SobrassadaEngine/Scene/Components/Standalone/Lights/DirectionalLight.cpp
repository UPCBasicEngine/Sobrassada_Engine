#include "DirectionalLight.h"
#include "Application.h"
#include "DebugDrawModule.h"
#include "Math/Quat.h"
#include "SceneModule.h"
#include "imgui.h"

DirectionalLight::DirectionalLight(UID uid, UID uidParent, UID uidRoot, const Transform& parentGlobalTransform)
    : LightComponent(uid, uidParent, uidRoot, "Directional Light", COMPONENT_DIRECTIONAL_LIGHT, parentGlobalTransform)
{
    direction = -float3::unitY;
    App->GetSceneModule()->GetLightsConfig()->AddDirectionalLight(this);
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::Render()
{
    if (!enabled || !drawGizmos) return;

    // Would be more optimal to only update the direction when rotation is modified
    float4x4 rot = float4x4::FromQuat(
        Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z)
    );
    direction              = (-float3::unitY * rot.RotatePart()).Normalized();

    DebugDrawModule* debug = App->GetDebugDrawModule();
    debug->DrawLine(globalTransform.position, direction, 2, float3(1, 1, 1));
}

void DirectionalLight::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Directional light parameters");
        ImGui::SliderFloat3("Direction ", &direction[0], -1.0, 1.0);
    }
}
