#pragma once

#include "Geometry/AABB.h"
#include "Geometry/LineSegment.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <vector>

class GameObject;
class FrustumPlanes;

constexpr float MinimumLeafSize = 1;

class Octree
{
  private:
    struct OctreeElement
    {
        size_t id = -1;
        AABB boundingBox;
        GameObject* gameObject = nullptr;

        OctreeElement(const AABB& boundingBox, GameObject* gameObject, size_t id)
            : boundingBox(boundingBox), gameObject(gameObject), id(id) {};

        bool operator==(const OctreeElement& otherElement) const { return id == otherElement.id; }

        bool operator<(const OctreeElement& otherElement) const { return id < otherElement.id; }
    };

    class OctreeNode
    {
      public:
        OctreeNode() = default;
        OctreeNode(const AABB& currentArea, int capacity) : currentArea(currentArea), elementsCapacity(capacity) {};

        ~OctreeNode();

        void Subdivide();
        bool Intersects(const AABB& elementBoundingBox) const { return currentArea.Intersects(elementBoundingBox); };
        bool IsLeaf() const { return topLeftFront == nullptr; };

        AABB currentArea;
        int elementsCapacity = 0;

        std::vector<OctreeElement> elements;

        OctreeNode* topLeftFront     = nullptr;
        OctreeNode* topRightFront    = nullptr;
        OctreeNode* bottomLeftFront  = nullptr;
        OctreeNode* bottomRightFront = nullptr;
        OctreeNode* topLeftBack      = nullptr;
        OctreeNode* topRightBack     = nullptr;
        OctreeNode* bottomLeftBack   = nullptr;
        OctreeNode* bottomRightBack  = nullptr;
    };

  public:
    Octree(const float3& position, float size, int capacity);
    ~Octree();

    bool InsertElement(GameObject* gameObject);
    void QueryElements(const AABB& area, std::vector<GameObject*>& foundElements) const;
    void QueryElements(const FrustumPlanes& cameraPlanes, std::vector<GameObject*>& foundElements) const;
    void GetDrawLines(std::vector<LineSegment>& drawLines, std::vector<LineSegment>& elementLines) const;

  private:
    OctreeNode* rootNode;

    int totalLeaf     = 0;
    int totalElements = 0;
};
