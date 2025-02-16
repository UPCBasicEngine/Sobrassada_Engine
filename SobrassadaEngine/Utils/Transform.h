#pragma once

#include <Math/float3.h>

enum TransformType
{
    LOCAL = 0,
    GLOBAL = 1
};

enum PivotType
{
    OBJECT = 0,
    ROOT = 1
};

struct Transform 
{
    Transform() : position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}

    Transform(const float3& position, const float3& rotation, const float3& scale): position(position), rotation(rotation), scale(scale) {}

    Transform(const Transform& other) = default;

public:

    Transform operator+(const Transform &transform) const
    {
        return {position + transform.position, rotation + transform.rotation,
            float3(scale.x * transform.scale.x, scale.y * transform.scale.y, scale.z * transform.scale.z)};
    }

    Transform operator-(const Transform &transform) const
    {
        float3 newScale;
        newScale.x = transform.scale.x != 0 ? scale.x / transform.scale.x : 0;  
        newScale.y = transform.scale.y != 0 ? scale.y / transform.scale.y : 0;  
        newScale.z = transform.scale.z != 0 ? scale.z / transform.scale.z : 0;  
        
        return {position - transform.position, rotation - transform.rotation, newScale};
    }

    void Set(const Transform &transform);
    
public:
    
    float3 position;
    float3 rotation;
    float3 scale;

    static const Transform identity;
};
