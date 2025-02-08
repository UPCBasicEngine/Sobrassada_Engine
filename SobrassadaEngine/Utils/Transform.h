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

    Transform(const float3& position, const float3& rotation, const float3& scale): position(position), rotation(rotation), scale(scale) {}

public:

    Transform operator+(const Transform &transform) const
    {
        return {position + transform.position, rotation + transform.rotation, scale * transform.scale};
    }

    Transform operator-(const Transform &transform) const
    {
        return {position - transform.position, rotation - transform.rotation, scale / transform.scale};
    }

    void Set(const Transform &transform)
    {
        this->position = float3(transform.position);
        this->rotation = float3(transform.rotation);
        this->scale = float3(transform.scale);
    }
    
public:
    
    float3 position;
    float3 rotation;
    float3 scale;
};
