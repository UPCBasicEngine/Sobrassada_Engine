#pragma once

#include "Component.h"
#include "Application.h"

#include "Math/float3.h"

class LightComponent : public Component
{
  public:
    LightComponent();
    ~LightComponent();

    float GetIntensity() const { return intensity; }
    float3 GetColor() const { return color; }

    void SetIntensity(const float newIntensity) { intensity = newIntensity; }
    void SetColor(const float3& newColor) { color = newColor; }

  protected:
    float intensity;
    float3 color;

    bool drawGizmos;
};
