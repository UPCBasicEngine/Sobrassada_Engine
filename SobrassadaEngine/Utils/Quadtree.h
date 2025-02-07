#pragma once

#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <set>
#include <vector>

constexpr int MinimumLeafSize = 2;

struct Box
{
    float x = 0;
    float y = 0;

    float sizeX = 0;
    float sizeY = 0;

    Box() = default;
    Box(float x, float y, float sizeX, float sizeY) : x(x), y(y), sizeX(sizeX), sizeY(sizeY) {};

    bool operator==(const Box &otherBox) const
    {
        return (x == otherBox.x && y == otherBox.y && sizeX == otherBox.sizeX && sizeY == otherBox.sizeY);
    }

    bool operator<(const Box& otherBox) const
    {
        if (x < otherBox.x) return true;
        else if (x == otherBox.x && y < otherBox.y) return true;

        return false;
    }
};

class Quadtree
{
  private:
    class QuadtreeNode
    {
      public:
        QuadtreeNode() = default;
        QuadtreeNode(float2 position, float size, int capacity)
            : position(position), size(size), elementsCapacity(capacity) {};

        ~QuadtreeNode();

        void Subdivide();
        bool Intersects(const Box &element) const;
        bool IsLeaf() const { return topLeft == nullptr; };

        float2 position      = float2(0, 0);
        float size           = 1;
        int elementsCapacity = 0;

        std::vector<Box> elements;

        QuadtreeNode *topLeft     = nullptr;
        QuadtreeNode *topRight    = nullptr;
        QuadtreeNode *bottomLeft  = nullptr;
        QuadtreeNode *bottomRight = nullptr;
    };

  public:
    Quadtree(float2 position, float size, int capacity);
    ~Quadtree();

    bool InsertElement(const Box &newElement);
    void QueryElements(const Box &area, std::set<Box> &foundElements) const;
    void GetDrawLines(std::vector<float4> &drawLines, std::vector<float4> &elementLines) const;

  private:
    QuadtreeNode *rootNode;

    int totalLeaf     = 0;
    int totalElements = 0;
};