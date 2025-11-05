#pragma once

#include "Globals.h"
#include "Octree.h"
#include "Quadtree.h"
#include "DynamicOctree.h"

#include <vector>

namespace math
{
    class LineSegment;
}

class GameObject;
struct BulletUserPointer;

namespace RaycastController
{
    SOBRASADA_API_ENGINE GameObject*
    GetRayIntersectionObject(const math::LineSegment& ray, const std::vector<GameObject*>& queriedGameObjects);

    SOBRASADA_API_ENGINE BulletUserPointer* GetRayIntersectionPhysics(const math::LineSegment& ray);

    template <typename... Tree> GameObject* GetRayIntersectionTrees(const math::LineSegment& ray, const Tree*... trees)
    {
        std::vector<GameObject*> queriedGameObjects;
        (trees->template QueryElements<math::LineSegment>(ray, queriedGameObjects), ...);

        return GetRayIntersectionObject(ray, queriedGameObjects);
    }
} // namespace RaycastController