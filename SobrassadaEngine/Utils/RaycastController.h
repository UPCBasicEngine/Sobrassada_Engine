#pragma once

#include "Globals.h"
#include "Octree.h"
#include "Quadtree.h"

#include <vector>

namespace math
{
    class LineSegment;
}

class GameObject;

namespace RaycastController
{
    SOBRASADA_API_ENGINE GameObject*
    GetRayIntersectionObject(const math::LineSegment& ray, const std::vector<GameObject*>& queriedGameObjects);

    template <typename... Tree> GameObject* GetRayIntersectionTrees(const math::LineSegment& ray, const Tree*... trees)
    {
        std::vector<GameObject*> queriedGameObjects;
        (trees->template QueryElements<math::LineSegment>(ray, queriedGameObjects), ...);

        return GetRayIntersectionObject(ray, queriedGameObjects);
    }
} // namespace RaycastController