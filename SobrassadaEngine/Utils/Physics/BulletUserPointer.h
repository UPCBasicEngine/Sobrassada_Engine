#pragma once

#include "ComponentUtils.h"
#include "Delegate.h"

#include "Math/float3.h"

class Component;
class GameObject;

struct BulletUserPointer
{
    BulletUserPointer(Component* component, CollisionDelegate* newCallback, bool generateCallback, ColliderLayer newLayer)
    {
        collider            = component;
        onCollisionCallback = newCallback;
        this->generateCallback = generateCallback;
        layer = newLayer;
    }

    Component* collider;
    CollisionDelegate* onCollisionCallback;
    ColliderLayer layer;
    bool generateCallback;
};