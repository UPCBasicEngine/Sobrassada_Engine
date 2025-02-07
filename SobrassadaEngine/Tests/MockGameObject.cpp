#include "MockGameObject.h"

MockGameObject::MockGameObject(const float3 &minPoint, const float3 &maxPoint)
{
    boundingBox = AABB(minPoint, maxPoint);
}

MockGameObject::~MockGameObject()
{
}
