#pragma once

#include "LightComponent.h"

class SpotLight : public LightComponent
{
  public:
    SpotLight();
    SpotLight(const float3 &position, const float3 &direction);
    ~SpotLight();

    void EditorParams(const int index);
    void DrawGizmos() const;

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