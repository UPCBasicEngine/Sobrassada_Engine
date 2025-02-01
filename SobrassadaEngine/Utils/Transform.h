#pragma once
#include <Math/float3.h>

enum TransformType
{
    LOCAL = 0,
    GLOBAL = 1
};

struct Transform 
{
    Transform() : position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}

public:
    
    float3 position;
    float3 rotation;
    float3 scale;
};
