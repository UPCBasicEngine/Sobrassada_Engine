#pragma once

#include "../LightComponent.h"

#include "rapidjson/document.h"

class SpotLightComponent : public LightComponent
{
  public:
    static const ComponentType STATIC_TYPE = ComponentType::COMPONENT_SPOT_LIGHT;

    SpotLightComponent(UID uid, GameObject* parent);
    SpotLightComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~SpotLightComponent() override;

    void Init() override;

    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    const float3 GetDirection();
    float GetRange() const { return range; }
    float GetInnerAngle() const { return innerAngle; }
    float GetOuterAngle() const { return outerAngle; }

  private:
    float range;
    float innerAngle;
    float outerAngle;
};