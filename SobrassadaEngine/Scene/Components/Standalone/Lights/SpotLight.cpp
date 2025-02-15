#include "SpotLight.h"

#include "DebugDrawModule.h"

#include "ImGui.h"
#include "Math/Quat.h"

SpotLight::SpotLight(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform)
    : LightComponent(uid, uidParent, uidRoot, "Spot Light", COMPONENT_SPOT_LIGHT, parentGlobalTransform)
{
    direction  = -float3::unitY;
    range      = 3;
    innerAngle = 10;
    outerAngle = 20;
}

// SpotLight::SpotLight(const float3 &position, const float3 &direction) : LightComponent()
//{
//     this->position  = position;
//     this->direction = direction;
//     range           = 3;
//     innerAngle      = 10;
//     outerAngle      = 20;
// }

SpotLight::~SpotLight() {}

void SpotLight::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Spot light parameters");

        ImGui::SliderFloat3("Color", &color[0], 0.0f, 1.0f);

        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f);
        ImGui::SliderFloat("Range", &range, 0.0f, 10.0f);
        if (ImGui::SliderFloat("Inner angle", &innerAngle, 0.0f, 90.0f))
        {
            if (innerAngle > outerAngle) outerAngle = innerAngle;
        }
        if (ImGui::SliderFloat("Outer angle", &outerAngle, 0.0f, 90.0f))
        {
            if (outerAngle < innerAngle) innerAngle = outerAngle;
        }

        ImGui::Checkbox("Draw gizmos", &drawGizmos);
    }
}

void SpotLight::Render()
{
    if (!enabled || !drawGizmos) return;

    const float innerRads      = innerAngle * (PI / 180.0f) > PI / 2 ? PI / 2 : innerAngle * (PI / 180.0f);
    const float outerRads      = outerAngle * (PI / 180.0f) > PI / 2 ? PI / 2 : outerAngle * (PI / 180.0f);

    float4x4 rot = float4x4::FromQuat(
        Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z)
    );
    direction = (-float3::unitY * rot.RotatePart()).Normalized();    

    std::vector<float3> innerDirections;
    innerDirections.push_back(float3(Quat::RotateX(innerRads).Transform(direction)));
    innerDirections.push_back(float3(Quat::RotateX(-innerRads).Transform(direction)));

    std::vector<float3> outerDirections;
    outerDirections.push_back(float3(Quat::RotateZ(outerRads).Transform(direction)));
    outerDirections.push_back(float3(Quat::RotateZ(-outerRads).Transform(direction)));

    DebugDrawModule *debug = App->GetDebugDrawModule();
    debug->DrawLine(globalTransform.position, direction, range, float3(1, 1, 1));

    for (const float3 &dir : innerDirections)
    {
        debug->DrawLine(globalTransform.position, dir, range / cos(innerRads), float3(1, 1, 1));
    }

    for (const float3 &dir : outerDirections)
    {
        debug->DrawLine(globalTransform.position, dir, range / cos(outerRads), float3(1, 1, 1));
    }

    float3 center       = globalTransform.position + (direction * range);
    float innerCathetus = range * tan(innerRads);
    float outerCathetus = range * tan(outerRads);
    debug->DrawCircle(center, -direction, float3(1, 1, 1), innerCathetus);
    debug->DrawCircle(center, -direction, float3(1, 1, 1), outerCathetus);
}