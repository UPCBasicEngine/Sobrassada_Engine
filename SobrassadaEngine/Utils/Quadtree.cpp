#include "Quadtree.h"

#include "MockGameObject.h"

#include <stack>

Quadtree::Quadtree(const float2 &position, float size, int capacity)
{
    float halfSize         = size / 2.f;
    float2 minPosition     = float2(position.x - halfSize, position.y - halfSize);
    float2 maxPosition     = float2(position.x + halfSize, position.y + halfSize);
    AABB2D nodeBoundingBox = AABB2D(minPosition, maxPosition);

    rootNode               = new QuadtreeNode(nodeBoundingBox, capacity);
    totalLeaf              = 1;
}

Quadtree::~Quadtree()
{
    delete rootNode;
}

bool Quadtree::InsertElement(const MockGameObject *gameObject)
{
    if (gameObject == nullptr) return false;

    bool inserted = false;
    std::stack<QuadtreeNode *> nodesToVisit;
    nodesToVisit.push(rootNode);

    const AABB elementBoundingBox   = gameObject->GetWorldBoundingBox();
    AABB2D quadtreeBoundingBox      = AABB2D(elementBoundingBox.minPoint.xz(), elementBoundingBox.maxPoint.xz());
    QuadtreeElement quadtreeElement = QuadtreeElement(quadtreeBoundingBox, gameObject, totalElements);

    while (!nodesToVisit.empty())
    {
        QuadtreeNode *currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->Intersects(quadtreeBoundingBox))
        {
            if (currentNode->IsLeaf())
            {
                if (currentNode->currentArea.Width() <= MinimumLeafSize)
                {
                    currentNode->elements.push_back(quadtreeElement);
                    inserted = true;
                }
                else if (currentNode->elements.size() < currentNode->elementsCapacity)
                {
                    currentNode->elements.push_back(quadtreeElement);
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

void Quadtree::QueryElements(const AABB &area, std::vector<const MockGameObject *> &foundElements) const
{
    std::vector<bool> insertedElements = std::vector<bool>(totalElements, false);
    AABB2D area2D                      = AABB2D(area.minPoint.xz(), area.maxPoint.xz());

    std::stack<const QuadtreeNode *> nodesToVisit;
    nodesToVisit.push(rootNode);

    while (!nodesToVisit.empty())
    {
        const QuadtreeNode *currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->Intersects(area2D))
        {
            if (currentNode->IsLeaf())
            {
                for (const auto &element : currentNode->elements)
                {
                    if (!insertedElements[element.id])
                    {
                        insertedElements[element.id] = true;
                        foundElements.push_back(element.gameObject);
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
}

void Quadtree::GetDrawLines(std::vector<float4> &drawLines, std::vector<float4> &elementLines) const
{
    std::set<QuadtreeElement> includedElement;
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
            float halfSize = currentNode->currentArea.Width() / 2.f;

            float2 topLeft = float2(
                currentNode->currentArea.CenterPoint().x - halfSize, currentNode->currentArea.CenterPoint().y + halfSize
            );
            float2 topRight = float2(
                currentNode->currentArea.CenterPoint().x + halfSize, currentNode->currentArea.CenterPoint().y + halfSize
            );
            float2 bottomLeft = float2(
                currentNode->currentArea.CenterPoint().x - halfSize, currentNode->currentArea.CenterPoint().y - halfSize
            );
            float2 bottomRight = float2(
                currentNode->currentArea.CenterPoint().x + halfSize, currentNode->currentArea.CenterPoint().y - halfSize
            );

            drawLines[currentDrawLine++] = float4(topLeft.x, topLeft.y, topRight.x, topRight.y);
            drawLines[currentDrawLine++] = float4(topRight.x, topRight.y, bottomRight.x, bottomRight.y);
            drawLines[currentDrawLine++] = float4(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y);
            drawLines[currentDrawLine++] = float4(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y);

            for (auto &element : currentNode->elements)
            {
                if (includedElement.find(element) == includedElement.end())
                {
                    includedElement.insert(element);

                    float halfSizeX                    = element.boundingBox.Width() / 2.f;
                    float halfSizeY                    = element.boundingBox.Height() / 2.f;
                    float2 centerPoint                 = element.boundingBox.CenterPoint();

                    topLeft                            = float2(centerPoint.x - halfSizeX, centerPoint.y + halfSizeY);
                    topRight                           = float2(centerPoint.x + halfSizeX, centerPoint.y + halfSizeY);
                    bottomLeft                         = float2(centerPoint.x - halfSizeX, centerPoint.y - halfSizeY);
                    bottomRight                        = float2(centerPoint.x + halfSizeX, centerPoint.y - halfSizeY);

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
    float childSize        = currentArea.Width() / 2.f;
    float2 center          = currentArea.CenterPoint();

    float2 topPoint        = center + float2(0, childSize);
    float2 topRightPoint   = center + float2(childSize);
    float2 rightPoint      = center + float2(childSize, 0);
    float2 bottomPoint     = center + float2(0, -childSize);
    float2 bottomLeftPoint = center + float2(-childSize);
    float2 leftPoint       = center + float2(-childSize, 0);

    topLeft                = new QuadtreeNode(AABB2D(leftPoint, topPoint), elementsCapacity);
    topRight               = new QuadtreeNode(AABB2D(center, topRightPoint), elementsCapacity);
    bottomLeft             = new QuadtreeNode(AABB2D(bottomLeftPoint, center), elementsCapacity);
    bottomRight            = new QuadtreeNode(AABB2D(bottomPoint, rightPoint), elementsCapacity);

    for (auto &element : elements)
    {
        if (topLeft->Intersects(element.boundingBox)) topLeft->elements.push_back(element);
        if (topRight->Intersects(element.boundingBox)) topRight->elements.push_back(element);
        if (bottomLeft->Intersects(element.boundingBox)) bottomLeft->elements.push_back(element);
        if (bottomRight->Intersects(element.boundingBox)) bottomRight->elements.push_back(element);
    }

    elements.clear();
}