#pragma once

#include "LightComponent.h"

class SpotLight : public LightComponent
{
  public:
    SpotLight();
    ~SpotLight();

    void EditorParams();

    float3 GetPosition() const { return position; }
    float3 GetDirection() const { return direction; }
    float GetRange() const { return range; }
    float GetInnerAngle() const { return innerAngle; }
    float GetOuterAngle() const { return outerAngle; }

  private:
    float3 position;
    float3 direction;
    float range;
    float innerAngle;
    float outerAngle;
};