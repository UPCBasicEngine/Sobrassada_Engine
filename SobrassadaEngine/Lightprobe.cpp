#include "Lightprobe.h"
#include <algorithm>
Lightprobe::Lightprobe()
{
}
Lightprobe::~Lightprobe()
{
}

float Lightprobe::GetInfluence(const float3& worldPos) const
{
    AABB bounds = GetBounds();
    if (!bounds.Contains(worldPos)) return 0.0f;

    float3 center = bounds.CenterPoint();
    float3 halfSize = bounds.HalfSize();
    float3 localPos = (worldPos - center).Abs();

    float3 edgeDistance = halfSize - localPos;
    float minEdge       = std::min(std::min(edgeDistance.x, edgeDistance.y), edgeDistance.z);
    float maxHalfSize   = std::max(std::max(halfSize.x, halfSize.y), halfSize.z);
    return std::max(0.0f, minEdge / maxHalfSize);
}

AABB Lightprobe::GetBounds() const
{
    float3 halfSize = size * 0.5f;
    return AABB(position - halfSize, position + halfSize);
}
