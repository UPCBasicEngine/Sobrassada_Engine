#pragma once

#include "LightComponent.h"
#include "Math/float4.h"

namespace Lights
{
    struct SpotLightData
    {
        float4 position;
        float4 color;
        float3 direction;
        float innerAngle;
        float outerAngle;

        SpotLightData(const float4 &pos, const float4 &color, const float3 &dir, const float inner, const float outer) : 
            position(pos), color(color), direction(dir), innerAngle(inner), outerAngle(outer)
        {}
    };
} // namespace Lights

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