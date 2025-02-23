#include "SpotLightComponent.h"

#include "DebugDrawModule.h"
#include "Application.h"
#include "SceneModule.h"

#include "ImGui.h"
#include "Math/Quat.h"

SpotLightComponent::SpotLightComponent(UID uid, UID uidParent, UID uidRoot, const Transform &parentGlobalTransform)
    : LightComponent(uid, uidParent, uidRoot, "Spot Light", COMPONENT_SPOT_LIGHT, parentGlobalTransform)
{
    direction  = -float3::unitY;
    range      = 3;
    innerAngle = 10;
    outerAngle = 20;
    
    LightsConfig* lightsConfig = App->GetSceneModule()->GetLightsConfig();
    if (lightsConfig != nullptr) lightsConfig->AddSpotLight(this);
}

SpotLightComponent::SpotLightComponent(const rapidjson::Value& initialState) : LightComponent(initialState)
{
    direction = -float3::unitY;
    if (initialState.HasMember("Range"))
    {
        range = initialState["Range"].GetFloat();
    }
    if (initialState.HasMember("InnerAngle"))
    {
        innerAngle = initialState["InnerAngle"].GetFloat();
    }
    if (initialState.HasMember("OuterAngle"))
    {
        outerAngle = initialState["OuterAngle"].GetFloat();
    }

    LightsConfig* lightsConfig = App->GetSceneModule()->GetLightsConfig();
    if (lightsConfig != nullptr) lightsConfig->AddSpotLight(this);
}

SpotLightComponent::~SpotLightComponent()
{
    App->GetSceneModule()->GetLightsConfig()->RemoveSpotLight(uid);
}

void SpotLightComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    LightComponent::Save(targetState, allocator);

    targetState.AddMember("Range", range, allocator);
    targetState.AddMember("InnerAngle", innerAngle, allocator);
    targetState.AddMember("OuterAngle", outerAngle, allocator);
}

void SpotLightComponent::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    if (enabled)
    {
        ImGui::Text("Spot light parameters");

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

void SpotLightComponent::Render()
{
    if (!enabled || !drawGizmos) return;

    const float innerRads      = innerAngle * (PI / 180.0f) > PI / 2 ? PI / 2 : innerAngle * (PI / 180.0f);
    const float outerRads      = outerAngle * (PI / 180.0f) > PI / 2 ? PI / 2 : outerAngle * (PI / 180.0f);

    std::vector<float3> innerDirections;
    innerDirections.push_back(float3(Quat::RotateX(innerRads).Transform(-float3::unitY)));
    innerDirections.push_back(float3(Quat::RotateX(-innerRads).Transform(-float3::unitY)));

    std::vector<float3> outerDirections;
    outerDirections.push_back(float3(Quat::RotateZ(outerRads).Transform(-float3::unitY)));
    outerDirections.push_back(float3(Quat::RotateZ(-outerRads).Transform(-float3::unitY)));

    // Would be more optimal to only update the direction when rotation is modified
    float4x4 rot = float4x4::FromQuat(
        Quat::FromEulerXYZ(globalTransform.rotation.x, globalTransform.rotation.y, globalTransform.rotation.z)
    );
    direction = (rot.RotatePart() * -float3::unitY).Normalized(); 
    innerDirections[0]     = (rot.RotatePart() * innerDirections[0]); 
    innerDirections[1]     = (rot.RotatePart() * innerDirections[1]); 

    outerDirections[0]     = (rot.RotatePart() * outerDirections[0]); 
    outerDirections[1]     = (rot.RotatePart() * outerDirections[1]); 

    

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