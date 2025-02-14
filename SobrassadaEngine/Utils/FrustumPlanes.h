#pragma once

#include "Math/float4.h"

#include <vector>

constexpr int TOTAL_PLANES = 6;

namespace math
{
class float4x4;
class float3;
class AABB;
class OBB;
} // namespace math

class FrustumPlanes
{
  public:
    FrustumPlanes();
    ~FrustumPlanes();

    void UpdateFrustumPlanes(const float4x4 &viewMatrix, const float4x4 &projectionMatrix);

    bool Intersects(const AABB &boundingBox) const;
    bool Intersects(const OBB &boundingBox) const;

  private:
    bool CheckInsideFrustum(const float3 (&corners)[8]) const;
    bool PointInPlane(const float3 &point, const float4 &plane) const;

  private:
    std::vector<float4> frustumPlanes;
};
