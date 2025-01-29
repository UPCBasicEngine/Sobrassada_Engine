#pragma once

#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"

#include <list>
#include <unordered_set>
#include <vector>

constexpr int MinimumLeafSize = 2;

// TODO: CHANGE TO TEMPLATE CLASS
// TODO: CHANGE TO OBB
class Quadtree
{
  public:
    Quadtree(float2 position, float size, int capacity);
    ~Quadtree();

    bool InsertElement(float4 &newElement);
    void QueryElements(float4 &area, std::unordered_set<float4> &foundElements) const;
    void GetDrawLines(std::list<float4> &drawLines, std::list<float4> &elementLines) const;

  private:
    void Subdivide();
    bool Intersects(float4 &element) const;
    bool IsLeaf() const { return topLeft == nullptr; };

  private:
    float2 position;
    float size;
    int elementsCapacity;

    // TODO: CHANGE TO TEMPLATE CLASS
    std::vector<float4> elements;

    Quadtree *topLeft     = nullptr;
    Quadtree *topRight    = nullptr;
    Quadtree *bottomLeft  = nullptr;
    Quadtree *bottomRight = nullptr;
};
