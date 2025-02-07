#include "Quadtree.h"

#include <stack>

Quadtree::Quadtree(float2 position, float size, int capacity)
{
    rootNode  = new QuadtreeNode(position, size, capacity);
    totalLeaf = 1;
}

Quadtree::~Quadtree()
{
    delete rootNode;
}

bool Quadtree::InsertElement(const Box &newElement)
{
    bool inserted = false;
    std::stack<QuadtreeNode *> nodesToVisit;
    nodesToVisit.push(rootNode);

    while (!nodesToVisit.empty())
    {
        QuadtreeNode *currentNode = nodesToVisit.top();
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
                else if (currentNode->elements.size() < currentNode->elementsCapacity)
                {
                    currentNode->elements.push_back(newElement);
                    inserted = true;
                }
                else
                {
                    currentNode->Subdivide();
                    totalLeaf += 3;
                }
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
    if (inserted) ++totalElements;
    return inserted;
}

void Quadtree::QueryElements(const Box &area, std::set<Box> &foundElements) const
{
    std::stack<const QuadtreeNode *> nodesToVisit;
    nodesToVisit.push(rootNode);

    while (!nodesToVisit.empty())
    {
        const QuadtreeNode *currentNode = nodesToVisit.top();
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

void Quadtree::GetDrawLines(std::vector<float4> &drawLines, std::vector<float4> &elementLines) const
{
    std::set<Box> includedElement;
    drawLines    = std::vector<float4>(totalLeaf * 4, float4(0, 0, 0, 0));
    elementLines = std::vector<float4>(totalElements * 4, float4(0, 0, 0, 0));

    std::stack<const QuadtreeNode *> nodesToVisit;
    nodesToVisit.push(rootNode);

    int currentDrawLine    = 0;
    int currentElementLine = 0;

    while (!nodesToVisit.empty())
    {
        const QuadtreeNode *currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->IsLeaf())
        {

            float halfSize     = currentNode->size / 2.f;

            float2 topLeft     = float2(currentNode->position.x - halfSize, currentNode->position.y + halfSize);
            float2 topRight    = float2(currentNode->position.x + halfSize, currentNode->position.y + halfSize);
            float2 bottomLeft  = float2(currentNode->position.x - halfSize, currentNode->position.y - halfSize);
            float2 bottomRight = float2(currentNode->position.x + halfSize, currentNode->position.y - halfSize);

            drawLines[currentDrawLine++] = float4(topLeft.x, topLeft.y, topRight.x, topRight.y);
            drawLines[currentDrawLine++] = float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
            drawLines[currentDrawLine++] = float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y);
            drawLines[currentDrawLine++] = float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y);

            for (auto &element : currentNode->elements)
            {
                if (includedElement.find(element) == includedElement.end())
                {
                    includedElement.insert(element);

                    float halfSizeX                    = element.sizeX / 2.f;
                    float halfSizeY                    = element.sizeY / 2.f;

                    topLeft                            = float2(element.x - halfSizeX, element.y + halfSizeY);
                    topRight                           = float2(element.x + halfSizeX, element.y + halfSizeY);
                    bottomLeft                         = float2(element.x - halfSizeX, element.y - halfSizeY);
                    bottomRight                        = float2(element.x + halfSizeX, element.y - halfSizeY);

                    elementLines[currentElementLine++] = float4(topLeft.x, topLeft.y, topRight.x, topRight.y);
                    elementLines[currentElementLine++] = float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
                    elementLines[currentElementLine++] =
                        float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y);
                    elementLines[currentElementLine++] = float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y);
                }
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

Quadtree::QuadtreeNode::~QuadtreeNode()
{

    delete topLeft;
    delete topRight;
    delete bottomLeft;
    delete bottomRight;
}

void Quadtree::QuadtreeNode::Subdivide()
{
    float childSize     = size / 2.f;
    float halfChildSize = size / 4.f;

    topLeft =
        new QuadtreeNode(float2(position.x - halfChildSize, position.y + halfChildSize), childSize, elementsCapacity);
    topRight =
        new QuadtreeNode(float2(position.x + halfChildSize, position.y + halfChildSize), childSize, elementsCapacity);
    bottomLeft =
        new QuadtreeNode(float2(position.x - halfChildSize, position.y - halfChildSize), childSize, elementsCapacity);
    bottomRight =
        new QuadtreeNode(float2(position.x + halfChildSize, position.y - halfChildSize), childSize, elementsCapacity);

    for (auto &element : elements)
    {
        if (topLeft->Intersects(element)) topLeft->elements.push_back(element);
        if (topRight->Intersects(element)) topRight->elements.push_back(element);
        if (bottomLeft->Intersects(element)) bottomLeft->elements.push_back(element);
        if (bottomRight->Intersects(element)) bottomRight->elements.push_back(element);
    }

    elements.clear();
}

bool Quadtree::QuadtreeNode::Intersects(const Box &element) const
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
