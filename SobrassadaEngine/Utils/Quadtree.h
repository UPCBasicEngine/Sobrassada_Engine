#pragma once

#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <list>
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

// TODO: CHANGE TO TEMPLATE CLASS
// TODO: CHANGE TO OBB
class Quadtree
{
  public:
    Quadtree(float2 position, float size, int capacity);
    ~Quadtree();

    bool InsertElement(const Box &newElement);
    void QueryElements(const Box &area, std::unordered_set<Box, BoxHash> &foundElements) const;
    void GetDrawLines(std::list<float4> &drawLines, std::list<float4> &elementLines) const;

  private:
    void Subdivide();
    bool Intersects(const Box &element) const;
    bool IsLeaf() const { return topLeft == nullptr; };

  private:
    float2 position;
    float size;
    int elementsCapacity;

    // TODO: CHANGE TO TEMPLATE CLASS
    std::vector<Box> elements;

    Quadtree *topLeft     = nullptr;
    Quadtree *topRight    = nullptr;
    Quadtree *bottomLeft  = nullptr;
    Quadtree *bottomRight = nullptr;
};
