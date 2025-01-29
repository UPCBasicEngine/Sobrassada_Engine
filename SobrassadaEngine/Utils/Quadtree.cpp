#include "Quadtree.h"

Quadtree::Quadtree(float2 position, float size, int capacity)
    : position(position), size(size), elementsCapacity(capacity)
{
}

Quadtree::~Quadtree()
{
    delete topLeft;
    delete topRight;
    delete bottomLeft;
    delete bottomRight;
}

bool Quadtree::InsertElement(float4 &newElement)
{
    if (!Intersects(newElement)) return false;

    if (IsLeaf())
    {
        if (size <= MinimumLeafSize)
        {
            elements.push_back(newElement);
            return true;
        }

        if (elements.size() < elementsCapacity)
        {
            elements.push_back(newElement);
            return true;
        }

        Subdivide();
    }

    bool inserted = false;

    if (topLeft->InsertElement(newElement)) inserted = true;
    else if (topRight->InsertElement(newElement)) inserted = true;
    else if (bottomLeft->InsertElement(newElement)) inserted = true;
    else if (bottomRight->InsertElement(newElement)) inserted = true;

    return inserted = true;
}

void Quadtree::QueryElements(float4 &area, std::unordered_set<float4> &foundElements)
{
}

void Quadtree::GetDrawLines(std::list<float4> &drawLines, std::list<float4> &elementLines)
{
    if (elements.size() > 0)
    {
        for (auto &element : elements)
        {
            float sizeX = element.z;
            float sizeY = element.w;

            elementLines.push_back(float4(element.x, element.y, element.x + sizeX, element.y));
            elementLines.push_back(float4(element.x + sizeX, element.y, element.x + sizeX, element.y - sizeY));
            elementLines.push_back(float4(element.x, element.y - sizeY, element.x + sizeX, element.y - sizeY));
            elementLines.push_back(float4(element.x, element.y, element.x, element.y - sizeY));
        }
    }

    if (IsLeaf())
    {
        drawLines.push_back(float4(position.x, position.y, position.x + size, position.y));
        drawLines.push_back(float4(position.x + size, position.y, position.x + size, position.y - size));
        drawLines.push_back(float4(position.x, position.y - size, position.x + size, position.y - size));
        drawLines.push_back(float4(position.x, position.y, position.x, position.y - size));
    }
    else
    {
        topLeft->GetDrawLines(drawLines, elementLines);
        topRight->GetDrawLines(drawLines, elementLines);
        bottomLeft->GetDrawLines(drawLines, elementLines);
        bottomRight->GetDrawLines(drawLines, elementLines);
    }
}

void Quadtree::Subdivide()
{
    float childSize = size / 2.f;

    topLeft         = new Quadtree(float2(position.x, position.y), childSize, elementsCapacity);
    topRight        = new Quadtree(float2(position.x + childSize, position.y), childSize, elementsCapacity);
    bottomLeft      = new Quadtree(float2(position.x, position.y - childSize), childSize, elementsCapacity);
    bottomRight     = new Quadtree(float2(position.x + childSize, position.y - childSize), childSize, elementsCapacity);

    for (auto &element : elements)
    {
        topLeft->InsertElement(element);
        topRight->InsertElement(element);
        bottomLeft->InsertElement(element);
        bottomRight->InsertElement(element);
    }

    elements.clear();
}

bool Quadtree::Intersects(float4 &element)
{
    float2 l1 = position;
    float2 r1 = float2(position.x + size, position.y - size);

    float2 l2 = float2(element.x, element.y);
    float2 r2 = float2(element.x + element.z, element.y - element.w);

    if (l1.x > r2.x || l2.x > r1.x) return false;

    if (r1.y > l2.y || r2.y > l1.y) return false;

    return true;
}
