#pragma once

#include "Geometry/AABB.h"

class MockGameObject
{
  public:
    MockGameObject() = default;
    MockGameObject(const float3 &minPoint, const float3 &maxPoint);
    ~MockGameObject();

    const AABB &GetWorldBoundingBox() const { return boundingBox; }

    bool operator<(const MockGameObject& otherGameObject) const
    {
        float3 myCenter    = boundingBox.CenterPoint();
        float3 otherCenter = otherGameObject.boundingBox.CenterPoint();
        
        if (myCenter.x < otherCenter.x) return true;
        else if (myCenter.x == otherCenter.x && myCenter.y < otherCenter.y) return true;
        else if (myCenter.x == otherCenter.x && myCenter.y == otherCenter.y && myCenter.z < otherCenter.z) return true;

        return false;
    }

    bool operator==(const MockGameObject& otherGameObject) const
    {
        float3 myCenter    = boundingBox.CenterPoint();
        float3 otherCenter = otherGameObject.boundingBox.CenterPoint();

        return myCenter.Equals(otherCenter) && boundingBox.Size().Equals(otherGameObject.boundingBox.Size());
    }

  private:
    AABB boundingBox = AABB();
};
