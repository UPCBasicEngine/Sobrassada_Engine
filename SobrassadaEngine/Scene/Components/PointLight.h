#pragma once

#include "LightComponent.h"
#include "Math/float4.h"

namespace Lights
{
    struct PointLightData
    {
        float4 position;
        float4 color;

        PointLightData(const float4 &pos, const float4 &color) : position(pos), color(color) {}
    };
} // namespace Lights

class PointLight : public LightComponent
{
  public:
    PointLight();
    PointLight(const float3 &position, const float range);
    ~PointLight();

    void EditorParams(int index, bool isFirst, bool isLast);
    void DrawGizmos() const;

    float3 GetPosition() const { return position; }
    float GetRange() const { return range; }

    void SetPosition(const float3 &newPos) { position = newPos; }
    void SetRange(const float newRange) { range = newRange; }

  private:
    float3 position;
    float range;
    int renderMode;
};
