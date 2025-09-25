#pragma once
#include "Math/float3.h"
#include "Geometry/AABB.h"
class Lightprobe
{
  public:
    Lightprobe();
    ~Lightprobe();
    float3 position;
    float3 size;
    unsigned int cubemapTexture = 0;
    int cubemapIndex            = -1; 
    bool needUpdate             = true;
    float GetInfluence(const float3& worldPos) const;
    AABB GetBounds() const;
  
};
