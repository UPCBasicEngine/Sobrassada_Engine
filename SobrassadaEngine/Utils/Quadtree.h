#pragma once

#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <unordered_set>
#include <vector>

constexpr int MinimumLeafSize = 2;

struct Box
{
    float x;
    float y;

    float sizeX;
    float sizeY;

    Box(float x, float y, float sizeX, float sizeY) : x(x), y(y), sizeX(sizeX), sizeY(sizeY) {};

    bool operator==(const Box &otherBox) const
    {
        return (x == otherBox.x && y == otherBox.y && sizeX == otherBox.sizeX && sizeY == otherBox.sizeY);
    }
};

struct BoxHash
{
    auto operator()(const Box &box) const -> size_t
    {
        return std::hash<float> {}(box.x) ^ std::hash<float> {}(box.y) ^ std::hash<float> {}(box.sizeX) ^
               std::hash<float> {}(box.sizeY);
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
    void QueryElements(const Box &area, std::unordered_set<Box, BoxHash> &foundElements) const;
    void GetDrawLines(std::vector<float4> &drawLines, std::vector<float4> &elementLines) const;

  private:
    QuadtreeNode *rootNode;

    int totalLeaf     = 0;
    int totalElements = 0;
};