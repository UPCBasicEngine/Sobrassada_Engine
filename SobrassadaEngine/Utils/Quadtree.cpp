#include "Quadtree.h"

#include <stack>

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

bool Quadtree::InsertElementRecursive(const Box &newElement)
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

        SubdivideRecursive();
    }

    bool inserted = false;

    if (topLeft->InsertElementRecursive(newElement)) inserted = true;
    if (topRight->InsertElementRecursive(newElement)) inserted = true;
    if (bottomLeft->InsertElementRecursive(newElement)) inserted = true;
    if (bottomRight->InsertElementRecursive(newElement)) inserted = true;

    return inserted = true;
}

void Quadtree::QueryElementsRecursive(const Box &area, std::unordered_set<Box, BoxHash> &foundElements) const
{
    if (!Intersects(area)) return;

    for (auto &element : elements)
    {
        foundElements.insert(element);
    }

    if (IsLeaf()) return;

    topLeft->QueryElementsRecursive(area, foundElements);
    topRight->QueryElementsRecursive(area, foundElements);
    bottomLeft->QueryElementsRecursive(area, foundElements);
    bottomRight->QueryElementsRecursive(area, foundElements);
}

void Quadtree::GetDrawLinesRecursive(std::list<float4> &drawLines, std::list<float4> &elementLines) const
{
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

        for (auto &element : elements)
        {
            float halfSizeX = element.sizeX / 2.f;
            float halfSizeY = element.sizeY / 2.f;

            topLeft         = float2(element.x - halfSizeX, element.y + halfSizeY);
            topRight        = float2(element.x + halfSizeX, element.y + halfSizeY);
            bottomLeft      = float2(element.x - halfSizeX, element.y - halfSizeY);
            bottomRight     = float2(element.x + halfSizeX, element.y - halfSizeY);

            elementLines.push_back(float4(topLeft.x, topLeft.y, topRight.x, topRight.y));
            elementLines.push_back(float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y));
            elementLines.push_back(float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y));
            elementLines.push_back(float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y));
        }
    }
    else
    {
        topLeft->GetDrawLinesRecursive(drawLines, elementLines);
        topRight->GetDrawLinesRecursive(drawLines, elementLines);
        bottomLeft->GetDrawLinesRecursive(drawLines, elementLines);
        bottomRight->GetDrawLinesRecursive(drawLines, elementLines);
    }
}

bool Quadtree::InsertElement(const Box &newElement)
{
    bool inserted = false;
    std::stack<Quadtree *> nodesToVisit;
    nodesToVisit.push(this);

    while (!nodesToVisit.empty())
    {
        Quadtree *currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->Intersects(newElement))
        {
            if (currentNode->IsLeaf())
            {
                if (currentNode->size <= MinimumLeafSize)
                {
                    currentNode->elements.push_back(newElement);
                    inserted = true;
                }
                else if (currentNode->elements.size() < elementsCapacity)
                {
                    currentNode->elements.push_back(newElement);
                    inserted = true;
                }
                else currentNode->Subdivide();
            }
            if (!currentNode->IsLeaf())
            {
                nodesToVisit.push(currentNode->topLeft);
                nodesToVisit.push(currentNode->topRight);
                nodesToVisit.push(currentNode->bottomLeft);
                nodesToVisit.push(currentNode->bottomRight);
            }
        }
    }

    return inserted;
}

void Quadtree::QueryElements(const Box &area, std::unordered_set<Box, BoxHash> &foundElements) const
{
    std::stack<const Quadtree *> nodesToVisit;
    nodesToVisit.push(this);

    while (!nodesToVisit.empty())
    {
        const Quadtree *currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->Intersects(area))
        {
            if (currentNode->IsLeaf())
            {
                for (auto &element : currentNode->elements)
                {
                    foundElements.insert(element);
                }
            }
            else
            {
                nodesToVisit.push(currentNode->topLeft);
                nodesToVisit.push(currentNode->topRight);
                nodesToVisit.push(currentNode->bottomLeft);
                nodesToVisit.push(currentNode->bottomRight);
            }
        }
    }
}

void Quadtree::GetDrawLines(std::list<float4> &drawLines, std::list<float4> &elementLines) const
{
    std::stack<const Quadtree *> nodesToVisit;
    nodesToVisit.push(this);

    while (!nodesToVisit.empty())
    {
        const Quadtree *currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->IsLeaf())
        {

            float halfSize     = currentNode->size / 2.f;

            float2 topLeft     = float2(currentNode->position.x - halfSize, currentNode->position.y + halfSize);
            float2 topRight    = float2(currentNode->position.x + halfSize, currentNode->position.y + halfSize);
            float2 bottomLeft  = float2(currentNode->position.x - halfSize, currentNode->position.y - halfSize);
            float2 bottomRight = float2(currentNode->position.x + halfSize, currentNode->position.y - halfSize);

            drawLines.push_back(float4(topLeft.x, topLeft.y, topRight.x, topRight.y));
            drawLines.push_back(float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y));
            drawLines.push_back(float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y));
            drawLines.push_back(float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y));

            for (auto &element : currentNode->elements)
            {
                float halfSizeX = element.sizeX / 2.f;
                float halfSizeY = element.sizeY / 2.f;

                topLeft         = float2(element.x - halfSizeX, element.y + halfSizeY);
                topRight        = float2(element.x + halfSizeX, element.y + halfSizeY);
                bottomLeft      = float2(element.x - halfSizeX, element.y - halfSizeY);
                bottomRight     = float2(element.x + halfSizeX, element.y - halfSizeY);

                elementLines.push_back(float4(topLeft.x, topLeft.y, topRight.x, topRight.y));
                elementLines.push_back(float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y));
                elementLines.push_back(float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y));
                elementLines.push_back(float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y));
            }
        }
        else
        {
            nodesToVisit.push(currentNode->topLeft);
            nodesToVisit.push(currentNode->topRight);
            nodesToVisit.push(currentNode->bottomLeft);
            nodesToVisit.push(currentNode->bottomRight);
        }
    }
}

void Quadtree::SubdivideRecursive()
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
        topLeft->InsertElementRecursive(element);
        topRight->InsertElementRecursive(element);
        bottomLeft->InsertElementRecursive(element);
        bottomRight->InsertElementRecursive(element);
    }

    elements.clear();
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
        if (topLeft->Intersects(element)) topLeft->elements.push_back(element);
        if (topRight->Intersects(element)) topRight->elements.push_back(element);
        if (bottomLeft->Intersects(element)) bottomLeft->elements.push_back(element);
        if (bottomRight->Intersects(element)) bottomRight->elements.push_back(element);
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
