#pragma once

#include "LightComponent.h"

class PointLight : public LightComponent
{
  public:
    PointLight();
    PointLight(const float3 &position, const float range);
    ~PointLight();

    void EditorParams(int index);
    void DrawGizmos() const;

    float3 GetPosition() const { return position; }
    float GetRange() const { return range; }

  private:
    float3 position;
    float range;
    int gizmosMode;
};
