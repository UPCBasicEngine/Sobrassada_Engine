#pragma once

#include "Geometry/AABB.h"

#include <set>
#include <stack>
#include <vector>

class GameObject;

struct DynamicOctreeElement
{
    AABB boundingBox;
    GameObject* gameObject = nullptr;

    DynamicOctreeElement(const AABB& boundingBox, GameObject* gameObject)
        : boundingBox(boundingBox), gameObject(gameObject) {};

    bool operator==(const DynamicOctreeElement& otherElement) const { return gameObject == otherElement.gameObject; }

    bool operator<(const DynamicOctreeElement& otherElement) const { return gameObject < otherElement.gameObject; }
};

struct DynamicOctreeNode
{
    DynamicOctreeNode() = default;
    DynamicOctreeNode(const AABB& currentArea, int capacity, DynamicOctreeNode* parent)
        : currentArea(currentArea), elementsCapacity(capacity), parentNode(parent) {};
    ~DynamicOctreeNode();

    void Subdivide();
    bool Intersects(const AABB& elementBoundingBox) const { return currentArea.Intersects(elementBoundingBox); };
    bool IsLeaf() const { return children[0] == nullptr; };

    bool InsertElement(DynamicOctreeElement& elementToAdd);
    bool RemoveElement(DynamicOctreeElement& elementToRemove);
    bool HasElement(const DynamicOctreeElement& elementToCheck) const;

    AABB currentArea;
    int elementsCapacity = 0;

    std::vector<DynamicOctreeElement> elements;

    DynamicOctreeNode* parentNode  = nullptr;
    DynamicOctreeNode* children[8] = {nullptr};
};

class DynamicOctree
{
  public:
    DynamicOctree(const float3& position, float size, int capacity);
    ~DynamicOctree();

    bool InsertElement(GameObject* gameObject);
    bool RemoveElement(GameObject* gameObject);
    const std::vector<LineSegment>& GetDrawLines();

    template <typename AreaType>
    void QueryElements(const AreaType& queryObject, std::vector<GameObject*>& foundElements) const;

  private:
    DynamicOctreeNode* rootNode = nullptr;

    int totalLeaf               = 0;
    int totalElements           = 0;

    std::vector<LineSegment> drawLines;
};

template <typename AreaType>
inline void DynamicOctree::QueryElements(const AreaType& queryObject, std::vector<GameObject*>& foundElements) const
{
#ifdef OPTICK
    OPTICK_CATEGORY("Octree::QueryElements", Optick::Category::GameLogic)
#endif
    std::stack<DynamicOctreeNode*> nodesToVisit;
    nodesToVisit.push(rootNode);

    while (!nodesToVisit.empty())
    {
        const DynamicOctreeNode* currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (queryObject.Intersects(currentNode->currentArea))
        {
            if (currentNode->IsLeaf())
            {
                for (const auto& element : currentNode->elements)
                {
                    foundElements.push_back(element.gameObject);
                }
            }
            else
            {
                for (DynamicOctreeNode* child : currentNode->children)
                    nodesToVisit.push(child);
            }
        }
    }
}