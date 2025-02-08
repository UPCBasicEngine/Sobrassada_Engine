#pragma once

#include "Component.h"
#include "Math/float4.h"

class LightComponent : public Component
{
  public:
    LightComponent();
    ~LightComponent();

    float GetIntensity() const { return intensity; }
    float4 GetColor() const { return color; }

    void SetIntensity(const float newIntensity) { intensity = newIntensity; }
    void SetColor(const float4 newColor) { color = newColor; }

  protected:
    float intensity;
    float4 color;
};
