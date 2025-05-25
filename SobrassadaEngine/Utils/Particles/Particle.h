#pragma once

#include "Math/float2.h"
#include "Math/float3.h"

struct Particle
{
    Particle() = default;
    Particle(float3 newPosition) : position(newPosition) {};

    float3 position   = float3::zero;
    float3 velocity   = float3::zero;
    float lifeTime    = 3.f;
    int tileOffset[2] = {0, 0};
    bool alive        = true;
};