#pragma once

#include "Geometry/AABB.h"
#include "Geometry/LineSegment.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <stack>
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
    void GetDrawLines(std::vector<LineSegment>& drawLines, std::vector<LineSegment>& elementLines) const;

    template <typename AreaType>
    void QueryElements(const AreaType& cameraPlanes, std::vector<GameObject*>& foundElements) const;

  private:
    OctreeNode* rootNode;

    int totalLeaf     = 0;
    int totalElements = 0;
};

template <typename AreaType>
inline void Octree::QueryElements(const AreaType& queryObject, std::vector<GameObject*>& foundElements) const
{
    std::vector<bool> insertedElements = std::vector<bool>(totalElements, false);

    std::stack<OctreeNode*> nodesToVisit;
    nodesToVisit.push(rootNode);

    while (!nodesToVisit.empty())
    {
        const OctreeNode* currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (queryObject.Intersects(currentNode->currentArea))
        {
            if (currentNode->IsLeaf())
            {
                for (const auto& element : currentNode->elements)
                {
                    if (!insertedElements[element.id])
                    {
                        insertedElements[element.id] = true;
                        foundElements.push_back(element.gameObject);
                    }
                }
            }
            else
            {
                nodesToVisit.push(currentNode->topLeftFront);
                nodesToVisit.push(currentNode->topRightFront);
                nodesToVisit.push(currentNode->bottomLeftFront);
                nodesToVisit.push(currentNode->bottomRightFront);
                nodesToVisit.push(currentNode->topLeftBack);
                nodesToVisit.push(currentNode->topRightBack);
                nodesToVisit.push(currentNode->bottomLeftBack);
                nodesToVisit.push(currentNode->bottomRightBack);
            }
        }
    }
}