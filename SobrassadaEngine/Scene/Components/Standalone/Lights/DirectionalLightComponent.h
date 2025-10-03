#pragma once

#include "../LightComponent.h"

#include "rapidjson/document.h"

class DirectionalLightComponent : public LightComponent
{
  public:
    static const ComponentType STATIC_TYPE = ComponentType::COMPONENT_DIRECTIONAL_LIGHT;

    DirectionalLightComponent(UID uid, GameObject* parent);
    DirectionalLightComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~DirectionalLightComponent() override;

    void Init() override;

    void RenderEditorInspector() override;
    void RenderDebug(float deltaTime) override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    const float3 GetDirection() const;
    const float3 GetShadowTint() const { return shadowTint; }
    const float GetShadowStrength() const { return shadowStrength; }

  private:
    float3 shadowTint;
    float shadowStrength = 1.0f;
};