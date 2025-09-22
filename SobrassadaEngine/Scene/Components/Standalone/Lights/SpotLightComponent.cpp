#include "SpotLightComponent.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "SceneModule.h"

#include "ImGui.h"
#include "Math/Quat.h"

SpotLightComponent::SpotLightComponent(UID uid, GameObject* parent)
    : LightComponent(uid, parent, "Spot Light", COMPONENT_SPOT_LIGHT)
{
    range                          = 3.f;
    innerAngle                     = 10.f;
    outerAngle                     = 20.f;

    const float4x4 globalTransform = GetGlobalTransform();
    spotCamera.type                = FrustumType::PerspectiveFrustum;
    spotCamera.pos                 = parent->GetGlobalPostition();

    spotCamera.front               = GetDirection();
    float3 tempVec                 = float3::unitX;
    spotCamera.up                  = Cross(spotCamera.front, tempVec).Normalized();

    spotCamera.nearPlaneDistance   = 0.1f;
    spotCamera.farPlaneDistance    = range;

    spotCamera.horizontalFov       = outerAngle * DEGREE_RAD_CONV;
    spotCamera.verticalFov         = outerAngle * DEGREE_RAD_CONV;
}

SpotLightComponent::SpotLightComponent(const rapidjson::Value& initialState, GameObject* parent)
    : LightComponent(initialState, parent)
{
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

    // if (!camera) camera = new CameraComponent(GenerateUID(), parent);

    const float4x4 globalTransform = GetGlobalTransform();
    spotCamera.type                = FrustumType::PerspectiveFrustum;
    spotCamera.pos                 = parent->GetGlobalPostition();

    spotCamera.front               = GetDirection();
    float3 tempVec                 = float3::unitX;
    spotCamera.up                  = -Cross(spotCamera.front, tempVec).Normalized();

    spotCamera.nearPlaneDistance   = 0.1f;
    spotCamera.farPlaneDistance    = range;

    spotCamera.horizontalFov       = outerAngle * DEGREE_RAD_CONV;
    spotCamera.verticalFov         = outerAngle * DEGREE_RAD_CONV;
}

SpotLightComponent::~SpotLightComponent()
{
    App->GetSceneModule()->GetScene()->GetLightsConfig()->RemoveSpotLight(this);

    // delete camera;
}

void SpotLightComponent::Init()
{
    App->GetSceneModule()->GetScene()->GetLightsConfig()->AddSpotLight(this);
}

void SpotLightComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    LightComponent::Save(targetState, allocator);

    targetState.AddMember("Range", range, allocator);
    targetState.AddMember("InnerAngle", innerAngle, allocator);
    targetState.AddMember("OuterAngle", outerAngle, allocator);
}

void SpotLightComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_SPOT_LIGHT)
    {
        const SpotLightComponent* otherLight = static_cast<const SpotLightComponent*>(other);
        enabled                              = otherLight->enabled;
        wasEnabled                           = otherLight->wasEnabled;

        intensity                            = otherLight->intensity;
        color                                = otherLight->color;
        drawGizmos                           = otherLight->drawGizmos;

        range                                = otherLight->range;
        innerAngle                           = otherLight->innerAngle;
        outerAngle                           = otherLight->outerAngle;

        // if (!camera) camera = new CameraComponent(GenerateUID(), parent);

        spotCamera.pos                       = parent->GetGlobalPostition();

        const float4x4 globalTransform       = GetGlobalTransform();
        spotCamera.front                     = GetDirection();
        float3 tempVec                       = float3::unitX;
        spotCamera.up                        = -Cross(spotCamera.front, tempVec).Normalized();

        spotCamera.nearPlaneDistance         = 0.1f;
        spotCamera.farPlaneDistance          = range;

        spotCamera.horizontalFov             = outerAngle * DEGREE_RAD_CONV;
        spotCamera.verticalFov               = outerAngle * DEGREE_RAD_CONV;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void SpotLightComponent::ParentUpdated()
{
    const float4x4 globalTransform = GetGlobalTransform();

    spotCamera.pos                 = parent->GetGlobalPostition();
    spotCamera.front               = GetDirection();
    float3 tempVec                 = float3::unitX;
    spotCamera.up                  = -Cross(spotCamera.front, tempVec).Normalized();
}

void SpotLightComponent::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    ImGui::Text("Spot light parameters");

    if (ImGui::SliderFloat("Range", &range, 0.0f, 10.0f))
    {
        spotCamera.farPlaneDistance = range;
    }

    if (ImGui::SliderFloat("Inner angle", &innerAngle, 0.0f, 90.0f))
    {
        if (innerAngle > outerAngle)
        {
            outerAngle               = innerAngle;
            spotCamera.horizontalFov = outerAngle * DEGREE_RAD_CONV;
            spotCamera.verticalFov   = outerAngle * DEGREE_RAD_CONV;
        }
    }
    if (ImGui::SliderFloat("Outer angle", &outerAngle, 0.0f, 90.0f))
    {
        if (outerAngle < innerAngle) innerAngle = outerAngle;

        spotCamera.horizontalFov = outerAngle * DEGREE_RAD_CONV;
        spotCamera.verticalFov   = outerAngle * DEGREE_RAD_CONV;
    }
}

void SpotLightComponent::RenderDebug(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (!drawGizmos || App->GetSceneModule()->GetInPlayMode()) return;

    const float innerRads = innerAngle * (PI / 180.0f) > PI / 2 ? PI / 2 : innerAngle * (PI / 180.0f);
    const float outerRads = outerAngle * (PI / 180.0f) > PI / 2 ? PI / 2 : outerAngle * (PI / 180.0f);

    std::vector<float3> innerDirections;
    innerDirections.emplace_back(Quat::RotateX(innerRads).Transform(-float3::unitY));
    innerDirections.emplace_back(Quat::RotateX(-innerRads).Transform(-float3::unitY));

    std::vector<float3> outerDirections;
    outerDirections.emplace_back(Quat::RotateZ(outerRads).Transform(-float3::unitY));
    outerDirections.emplace_back(Quat::RotateZ(-outerRads).Transform(-float3::unitY));

    const float4x4& globalTransform = parent->GetGlobalTransform();

    const float3 direction          = (globalTransform.RotatePart() * -float3::unitY).Normalized();
    innerDirections[0]              = (globalTransform.RotatePart() * innerDirections[0]);
    innerDirections[1]              = (globalTransform.RotatePart() * innerDirections[1]);
    outerDirections[0]              = (globalTransform.RotatePart() * outerDirections[0]);
    outerDirections[1]              = (globalTransform.RotatePart() * outerDirections[1]);

    DebugDrawModule* debug          = App->GetDebugDrawModule();
    debug->DrawLine(globalTransform.TranslatePart(), direction, range, float3(1, 1, 1));

    for (const float3& dir : innerDirections)
    {
        debug->DrawLine(globalTransform.TranslatePart(), dir, range / cos(innerRads), float3(1, 1, 1));
    }

    for (const float3& dir : outerDirections)
    {
        debug->DrawLine(globalTransform.TranslatePart(), dir, range / cos(outerRads), float3(1, 1, 1));
    }

    float3 center       = globalTransform.TranslatePart() + (direction * range);
    float innerCathetus = range * tan(innerRads);
    float outerCathetus = range * tan(outerRads);
    debug->DrawCircle(center, -direction, float3(1, 1, 1), innerCathetus);
    debug->DrawCircle(center, -direction, float3(1, 1, 1), outerCathetus);

    // RENDER LIGHT FRUSTUM
    debug->DrawFrustrum(spotCamera.ProjectionMatrix(), spotCamera.ViewMatrix());
}

const float3 SpotLightComponent::GetDirection()
{
    return (parent->GetGlobalTransform().RotatePart() * -float3::unitY).Normalized();
}
