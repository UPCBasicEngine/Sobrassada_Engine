#pragma once

#include "Application.h"
#include "Component.h"

#include "Math/float3.h"

class LightComponent : public Component
{
  public:
    LightComponent(UID uid, GameObject* parent, const char* uiName, ComponentType lightType);
    LightComponent(const rapidjson::Value& initialState, GameObject* parent);

    void Update(float deltaTime) override;
    virtual void RenderEditorInspector() override;
    virtual void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    virtual void Clone(const Component* other) = 0;

    float GetIntensity() const { return intensity; }
    float3 GetColor() const { return color; }

    void SetIntensity(const float newIntensity) { intensity = newIntensity; }
    void SetColor(const float3& newColor) { color = newColor; }

  protected:
    float intensity;
    float3 color;
    bool drawGizmos;
};
