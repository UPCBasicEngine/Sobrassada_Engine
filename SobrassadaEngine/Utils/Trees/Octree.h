#pragma once

#include "Geometry/AABB.h"

#include <stack>
#include <vector>
#ifdef OPTICK
#include "optick.h"
#endif

class GameObject;

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
    const std::vector<LineSegment>& GetDrawLines();

    template <typename AreaType>
    void QueryElements(const AreaType& queryObject, std::vector<GameObject*>& foundElements) const;

  private:
    OctreeNode* rootNode;

    int totalLeaf     = 0;
    int totalElements = 0;

    std::vector<LineSegment> drawLines;
};

template <typename AreaType>
inline void Octree::QueryElements(const AreaType& queryObject, std::vector<GameObject*>& foundElements) const
{
#ifdef OPTICK
    OPTICK_CATEGORY("Octree::QueryElements", Optick::Category::GameLogic)
#endif
    std::vector<uint8_t> insertedElements = std::vector<uint8_t>(totalElements, false);

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