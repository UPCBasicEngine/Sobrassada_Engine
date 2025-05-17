#pragma once

#include "btMotionState.h"

class Component;

namespace math
{
    class float3;
}

class BulletMotionState : public btMotionState
{
  public:
    BulletMotionState(
        Component* newCollider, const math::float3& newCenterOffset, const math::float3& newCenterRotation
    );
    ~BulletMotionState() = default;

    // Syncronize from render world to physics world
    void getWorldTransform(btTransform& outPhysicsWorldTransform) const override;

    // Syncronize from physics world to render world
    void setWorldTransform(const btTransform& physicsWorldTransform) override;

  private:
    Component* collider      = nullptr;
    btTransform centerOffset = btTransform::getIdentity();
};
