#pragma once

#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <utility>

struct Particle
{
    Particle() = default;
    Particle(float3 newPosition) : position(newPosition) {};

    float3 position                = float3::zero;
    float3 velocity                = float3::zero;
    float lifeTime                 = 3.f;
    std::pair<int, int> tileOffset = {0, 0};
    float4 color                   = float4::one;
    bool alive                     = true;
};