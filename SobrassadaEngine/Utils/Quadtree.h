#pragma once

#include "Geometry/AABB2D.h"
#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <set>
#include <vector>

class MockGameObject;

constexpr int MinimumLeafSize = 2;

class Quadtree
{
  private:
    struct QuadtreeElement
    {
        size_t id = -1;
        AABB2D boundingBox;
        const MockGameObject *gameObject = nullptr;

        QuadtreeElement(const AABB2D &boundingBox, const MockGameObject *gameObject, size_t id)
            : boundingBox(boundingBox), gameObject(gameObject), id(id) {};

        bool operator==(const QuadtreeElement &otherElement) const { return id == otherElement.id; }

        bool operator<(const QuadtreeElement &otherElement) const { return id < otherElement.id; }
    };

    class QuadtreeNode
    {
      public:
        QuadtreeNode() = default;
        QuadtreeNode(const AABB2D &currentArea, int capacity) : currentArea(currentArea), elementsCapacity(capacity) {};

        ~QuadtreeNode();

        void Subdivide();
        bool Intersects(const AABB2D &elementBoundingBox) const { return currentArea.Intersects(elementBoundingBox); };
        bool IsLeaf() const { return topLeft == nullptr; };

        AABB2D currentArea;
        int elementsCapacity = 0;

        std::vector<QuadtreeElement> elements;

        QuadtreeNode *topLeft     = nullptr;
        QuadtreeNode *topRight    = nullptr;
        QuadtreeNode *bottomLeft  = nullptr;
        QuadtreeNode *bottomRight = nullptr;
    };

  public:
    Quadtree(const float2 &position, float size, int capacity);
    ~Quadtree();

    bool InsertElement(const MockGameObject *newElement);
    void QueryElements(const AABB &area, std::vector<const MockGameObject *> &foundElements) const;
    void GetDrawLines(std::vector<float4> &drawLines, std::vector<float4> &elementLines) const;

  private:
    QuadtreeNode *rootNode;

    int totalLeaf     = 0;
    int totalElements = 0;
};