#include "DirectionalLightComponent.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "SceneModule.h"

DirectionalLightComponent::DirectionalLightComponent(UID uid, GameObject* parent)
    : LightComponent(uid, parent, "Directional Light", COMPONENT_DIRECTIONAL_LIGHT)
{
}

DirectionalLightComponent::DirectionalLightComponent(const rapidjson::Value& initialState, GameObject* parent)
    : LightComponent(initialState, parent)
{
}

DirectionalLightComponent::~DirectionalLightComponent()
{
    App->GetSceneModule()->GetScene()->GetLightsConfig()->RemoveDirectionalLight(this);
}

void DirectionalLightComponent::Init()
{
    App->GetSceneModule()->GetScene()->GetLightsConfig()->AddDirectionalLight(this);
}

void DirectionalLightComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    LightComponent::Save(targetState, allocator);
}

void DirectionalLightComponent::Clone(const Component* other)
{
    if (other->GetType() == ComponentType::COMPONENT_DIRECTIONAL_LIGHT)
    {
        const DirectionalLightComponent* otherLight = static_cast<const DirectionalLightComponent*>(other);
        enabled                                     = otherLight->enabled;
        wasEnabled                                  = otherLight->wasEnabled;

        intensity                                   = otherLight->intensity;
        color                                       = otherLight->color;
        drawGizmos                                  = otherLight->drawGizmos;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void DirectionalLightComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void DirectionalLightComponent::RenderDebug(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (!drawGizmos || App->GetSceneModule()->GetInPlayMode()) return;

    DebugDrawModule* debug = App->GetDebugDrawModule();
    debug->DrawLine(
        parent->GetGlobalTransform().TranslatePart(),
        (parent->GetGlobalTransform().RotatePart() * -float3::unitY).Normalized(), 2, float3(1, 1, 1)
    );
}

const float3 DirectionalLightComponent::GetDirection() const
{
    return (parent->GetGlobalTransform().RotatePart() * -float3::unitY).Normalized();
}
