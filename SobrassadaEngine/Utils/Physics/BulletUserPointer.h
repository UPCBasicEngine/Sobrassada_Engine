#pragma once

#include "ComponentUtils.h"
#include "Delegate.h"

#include "Math/float3.h"

class Component;
class GameObject;

struct SOBRASADA_API_ENGINE BulletUserPointer
{
    BulletUserPointer() = default;
    BulletUserPointer(
        Component* component, CollisionDelegate* onCollisionCall, CollisionDelegate* onCollisionEnterCall,
        CollisionExitDelegate* onCollisionExitCall, bool generateCallback, ColliderLayer newLayer
    )
    {
        collider                 = component;
        onCollisionCallback      = onCollisionCall;
        onCollisionEnterCallback = onCollisionEnterCall;
        onCollisionExitCallback  = onCollisionExitCall;
        this->generateCallback   = generateCallback;
        layer                    = newLayer;
    }

    BulletUserPointer(const BulletUserPointer& otherBulletPointer)
    {
        collider                 = otherBulletPointer.collider;
        onCollisionCallback      = otherBulletPointer.onCollisionCallback;
        onCollisionEnterCallback = otherBulletPointer.onCollisionEnterCallback;
        onCollisionExitCallback  = otherBulletPointer.onCollisionExitCallback;
        generateCallback         = otherBulletPointer.generateCallback;
        layer                    = otherBulletPointer.layer;
    }

    Component* collider;
    CollisionDelegate* onCollisionCallback;
    CollisionDelegate* onCollisionEnterCallback;
    CollisionExitDelegate* onCollisionExitCallback;
    ColliderLayer layer;
    bool generateCallback;
};