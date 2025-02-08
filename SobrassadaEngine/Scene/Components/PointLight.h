#pragma once

#include "LightComponent.h"

class PointLight : public LightComponent
{
  public:
    PointLight();
    ~PointLight();

    void EditorParams();
    void DrawGizmos() const;

    float3 GetPosition() const { return position; }
    float GetRange() const { return range; }

    void SetPosition(const float3 &newPos) { position = newPos; }
    void SetRange(const float newRange) { range = newRange; }

  private:
    float3 position;
    float range;

};
