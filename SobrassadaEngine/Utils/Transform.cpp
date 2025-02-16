
#include "Transform.h"

const Transform Transform::identity = Transform();

void Transform::Set(const Transform &transform)
{
    this->position = float3(transform.position);
    this->rotation = float3(transform.rotation);
    this->scale = float3(transform.scale);
}