#include "DirectionalLightComponent.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "SceneModule.h"
#include "imgui.h"

DirectionalLightComponent::DirectionalLightComponent(UID uid, GameObject* parent)
    : LightComponent(uid, parent, "Directional Light", COMPONENT_DIRECTIONAL_LIGHT)
{
    shadowTint = float3(0.9f, 0.9f, 0.9f);
}

DirectionalLightComponent::DirectionalLightComponent(const rapidjson::Value& initialState, GameObject* parent)
    : LightComponent(initialState, parent)
{

    if (initialState.HasMember("Shadow Tint"))
    {
        const rapidjson::Value& shadowArray = initialState["Shadow Tint"];
        shadowTint = {shadowArray[0].GetFloat(), shadowArray[1].GetFloat(), shadowArray[2].GetFloat()};
    }
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

    rapidjson::Value shadowTintArray(rapidjson::kArrayType);
    shadowTintArray.PushBack(shadowTint.x, allocator)
        .PushBack(shadowTint.y, allocator)
        .PushBack(shadowTint.z, allocator);

    targetState.AddMember("Shadow Tint", shadowTintArray, allocator);
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
        shadowTint                                  = otherLight->shadowTint;
    }
    else
    {
        GLOG("It is not possible to clone a component of a different type!");
    }
}

void DirectionalLightComponent::RenderEditorInspector()
{
    LightComponent::RenderEditorInspector();

    ImGui::DragFloat3("Shadow Tint", &shadowTint[0], 0.0f, 1.0f);
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
