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

bool Quadtree::InsertElement(const Box &newElement)
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
    if (topRight->InsertElement(newElement)) inserted = true;
    if (bottomLeft->InsertElement(newElement)) inserted = true;
    if (bottomRight->InsertElement(newElement)) inserted = true;

    return inserted = true;
}

void Quadtree::QueryElements(const Box &area, std::unordered_set<Box, BoxHash> &foundElements) const
{
    if (!Intersects(area)) return;

    for (auto &element : elements)
    {
        foundElements.insert(element);
    }

    if (IsLeaf()) return;

    topLeft->QueryElements(area, foundElements);
    topRight->QueryElements(area, foundElements);
    bottomLeft->QueryElements(area, foundElements);
    bottomRight->QueryElements(area, foundElements);
}

void Quadtree::GetDrawLines(std::list<float4> &drawLines, std::list<float4> &elementLines) const
{
    if (elements.size() > 0)
    {
        for (auto &element : elements)
        {
            float halfSizeX    = element.sizeX / 2.f;
            float halfSizeY    = element.sizeY / 2.f;

            float2 topLeft     = float2(element.x - halfSizeX, element.y + halfSizeY);
            float2 topRight    = float2(element.x + halfSizeX, element.y + halfSizeY);
            float2 bottomLeft  = float2(element.x - halfSizeX, element.y - halfSizeY);
            float2 bottomRight = float2(element.x + halfSizeX, element.y - halfSizeY);

            elementLines.push_back(float4(topLeft.x, topLeft.y, topRight.x, topRight.y));
            elementLines.push_back(float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y));
            elementLines.push_back(float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y));
            elementLines.push_back(float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y));
        }
    }

    if (IsLeaf())
    {
        float halfSize     = size / 2.f;

        float2 topLeft     = float2(position.x - halfSize, position.y + halfSize);
        float2 topRight    = float2(position.x + halfSize, position.y + halfSize);
        float2 bottomLeft  = float2(position.x - halfSize, position.y - halfSize);
        float2 bottomRight = float2(position.x + halfSize, position.y - halfSize);

        drawLines.push_back(float4(topLeft.x, topLeft.y, topRight.x, topRight.y));
        drawLines.push_back(float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y));
        drawLines.push_back(float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y));
        drawLines.push_back(float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y));
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
    float childSize     = size / 2.f;
    float halfChildSize = size / 4.f;

    topLeft = new Quadtree(float2(position.x - halfChildSize, position.y + halfChildSize), childSize, elementsCapacity);
    topRight =
        new Quadtree(float2(position.x + halfChildSize, position.y + halfChildSize), childSize, elementsCapacity);
    bottomLeft =
        new Quadtree(float2(position.x - halfChildSize, position.y - halfChildSize), childSize, elementsCapacity);
    bottomRight =
        new Quadtree(float2(position.x + halfChildSize, position.y - halfChildSize), childSize, elementsCapacity);

    for (auto &element : elements)
    {
        topLeft->InsertElement(element);
        topRight->InsertElement(element);
        bottomLeft->InsertElement(element);
        bottomRight->InsertElement(element);
    }

    elements.clear();
}

bool Quadtree::Intersects(const Box &element) const
{
    float halftSizeX          = size / 2.f;
    float halftSizeY          = size / 2.f;

    float2 selfTopLeft        = float2(position.x - halftSizeX, position.y + halftSizeY);
    float2 selfBottomRight    = float2(position.x + halftSizeX, position.y - halftSizeY);

    halftSizeX                = element.sizeX / 2.f;
    halftSizeY                = element.sizeY / 2.f;

    float2 elementTopLeft     = float2(element.x - halftSizeX, element.y + halftSizeY);
    float2 elementBottomRight = float2(element.x + halftSizeX, element.y - halftSizeY);

    if (selfTopLeft.x > elementBottomRight.x || elementTopLeft.x > selfBottomRight.x) return false;

    if (selfBottomRight.y > elementTopLeft.y || elementBottomRight.y > selfTopLeft.y) return false;

    return true;
}
