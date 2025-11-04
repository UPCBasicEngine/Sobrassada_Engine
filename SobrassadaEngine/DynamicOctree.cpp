#include "DynamicOctree.h"

#include "GameObject.h"

#include "Geometry/LineSegment.h"

#include <map>
#include <utility>

DynamicOctreeNode::~DynamicOctreeNode()
{
    for (auto& element : elements)
        element.gameObject->SetDynamicNode(nullptr);

    for (auto& child : children)
    {
        delete child;
        child = nullptr;
    }
}

void DynamicOctreeNode::Subdivide()
{
    float childSize          = currentArea.HalfSize().x;
    float3 center            = currentArea.CenterPoint();

    float3 pTop              = center + float3(0, childSize, 0);
    float3 pTopFront         = center + float3(0, childSize, childSize);
    float3 pTopRight         = center + float3(childSize, childSize, 0);
    float3 pTopFrontRight    = center + float3(childSize, childSize, childSize);
    float3 pBack             = center + float3(0, 0, -childSize);
    float3 pFront            = center + float3(0, 0, childSize);
    float3 pRight            = center + float3(childSize, 0, 0);
    float3 pLeft             = center + float3(-childSize, 0, 0);
    float3 pCenterBackLeft   = center + float3(-childSize, 0, -childSize);
    float3 pCenterFrontRight = center + float3(childSize, 0, childSize);
    float3 pBottom           = center + float3(0, -childSize, 0);
    float3 pBottomLeft       = center + float3(-childSize, -childSize, 0);
    float3 pBottomBack       = center + float3(0, -childSize, -childSize);
    float3 pBottomLeftBack   = center + float3(-childSize, -childSize, -childSize);

    children[0]              = new DynamicOctreeNode(AABB(pLeft, pTopFront), elementsCapacity, this);
    children[1]              = new DynamicOctreeNode(AABB(center, pTopFrontRight), elementsCapacity, this);
    children[2]              = new DynamicOctreeNode(AABB(pBottomLeft, pFront), elementsCapacity, this);
    children[3]              = new DynamicOctreeNode(AABB(pBottom, pCenterFrontRight), elementsCapacity, this);
    children[4]              = new DynamicOctreeNode(AABB(pCenterBackLeft, pTop), elementsCapacity, this);
    children[5]              = new DynamicOctreeNode(AABB(pBack, pTopRight), elementsCapacity, this);
    children[6]              = new DynamicOctreeNode(AABB(pBottomLeftBack, center), elementsCapacity, this);
    children[7]              = new DynamicOctreeNode(AABB(pBottomBack, pRight), elementsCapacity, this);

    // ADD ELEMENTS TO PROPER CHILD -> ONLY ONE REFERENCE!
    for (auto& element : elements)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i]->Intersects(element.boundingBox))
            {
                children[i]->InsertElement(element);
                break;
            }
        }
    }

    // REMOVE CURRENT REFERENCE TO GAME OBJECT IF NEEDED

    elements.clear();
}

bool DynamicOctreeNode::InsertElement(DynamicOctreeElement& elementToAdd)
{
    // ADD CURRENT REFERENCE TO GAME OBJECT IF NEEDED
    elementToAdd.gameObject->SetDynamicNode(this);

    elements.push_back(elementToAdd);

    return true;
}

bool DynamicOctreeNode::RemoveElement(DynamicOctreeElement& elementToRemove)
{
    // REMOVE CURRENT REFERENCE TO GAME OBJECT IF NEEDED

    int offset = -1;

    for (int i = 0; i < elements.size(); ++i)
    {
        if (elementToRemove == elements[i])
        {
            offset = i;
            break;
        }
    }

    if (offset > -1)
    {
        elementToRemove.gameObject->SetDynamicNode(nullptr);
        elements.erase(elements.begin() + offset);
        return true;
    }

    return false;
}

bool DynamicOctreeNode::HasElement(const DynamicOctreeElement& elementToCheck) const
{
    for (int i = 0; i < elements.size(); ++i)
    {
        if (elementToCheck == elements[i]) return true;
    }

    return false;
}

DynamicOctree::DynamicOctree(const float3& position, float size, int capacity)
{
    float halfSize       = size / 2.f;
    float3 minPosition   = float3(position.x - halfSize, position.y - halfSize, position.z - halfSize);
    float3 maxPosition   = float3(position.x + halfSize, position.y + halfSize, position.z + halfSize);

    AABB nodeBoundingBox = AABB(minPosition, maxPosition);

    rootNode             = new DynamicOctreeNode(nodeBoundingBox, capacity, nullptr);
    totalLeaf            = 1;
}

DynamicOctree::~DynamicOctree()
{
    delete rootNode;
}

bool DynamicOctree::InsertElement(GameObject* gameObject)
{
    if (gameObject == nullptr) return false;

    std::stack<DynamicOctreeNode*> nodesToVisit;
    nodesToVisit.push(rootNode);

    const AABB objectBoundingBox       = gameObject->GetGlobalAABB();
    DynamicOctreeElement octreeElement = DynamicOctreeElement(objectBoundingBox, gameObject);

    while (!nodesToVisit.empty())
    {
        DynamicOctreeNode* currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->Intersects(objectBoundingBox))
        {
            if (currentNode->IsLeaf())
            {
                if (currentNode->currentArea.HalfSize().x <= MINIMUM_TREE_LEAF_SIZE ||
                    currentNode->elements.size() < currentNode->elementsCapacity)
                {
                    if (currentNode->InsertElement(octreeElement))
                    {
                        ++totalElements;
                        return true;
                    }
                }
                else
                {
                    currentNode->Subdivide();
                    totalLeaf += 7;
                }
            }
            if (!currentNode->IsLeaf())
            {
                for (auto child : currentNode->children)
                    nodesToVisit.push(child);
            }
        }
    }

    return false;
}

bool DynamicOctree::RemoveElement(GameObject* gameObject, bool goTransformed)
{
    if (gameObject == nullptr) return false;

    bool removed = false;
    std::map<DynamicOctreeNode*, std::pair<bool, int>> nodeStates;

    DynamicOctreeNode* currentNode     = nullptr;

    const AABB objectBoundingBox       = gameObject->GetGlobalAABB();
    DynamicOctreeElement octreeElement = DynamicOctreeElement(objectBoundingBox, gameObject);

    if (goTransformed)
    {
        currentNode = gameObject->GetDynamicNode();
        if (currentNode && currentNode->RemoveElement(octreeElement))
        {
            removed = true;
            --totalElements;
            if (currentNode->elements.size() <= currentNode->elementsCapacity)
                nodeStates.insert({currentNode, std::make_pair(true, (int)currentNode->elements.size())});
            else nodeStates.insert({currentNode, std::make_pair(false, (int)currentNode->elements.size())});
        }
    }
    else
    {
        std::stack<DynamicOctreeNode*> nodesToVisit;
        nodesToVisit.push(rootNode);

        while (!nodesToVisit.empty() && !removed)
        {
            currentNode = nodesToVisit.top();
            nodesToVisit.pop();

            if (currentNode->Intersects(objectBoundingBox))
            {
                if (currentNode->IsLeaf())
                {
                    if (currentNode->HasElement(octreeElement))
                    {
                        --totalElements;
                        removed = currentNode->RemoveElement(octreeElement);

                        // SAVE CURRENT STATE TO LATER CHECK IF MERGEABLE
                        if (currentNode->elements.size() <= currentNode->elementsCapacity)
                            nodeStates.insert({currentNode, std::make_pair(true, (int)currentNode->elements.size())});
                        else nodeStates.insert({currentNode, std::make_pair(false, (int)currentNode->elements.size())});
                    }
                    else
                    {
                        if (currentNode->elements.size() <= currentNode->elementsCapacity)
                            nodeStates.insert({currentNode, std::make_pair(true, (int)currentNode->elements.size())});
                        else nodeStates.insert({currentNode, std::make_pair(false, (int)currentNode->elements.size())});
                    }
                }
                else
                {
                    for (auto child : currentNode->children)
                        nodesToVisit.push(child);
                }
            }
        }
    }

    // CHECK IF MERGEABLE
    auto nodeIterator = nodeStates.find(currentNode);
    if (nodeIterator != nodeStates.end() && nodeIterator->second.first && nodeIterator->first->parentNode)
    {
        std::stack<DynamicOctreeNode*> nodesToCheck;
        std::set<DynamicOctreeNode*> nodesInStack;
        nodesToCheck.push(nodeIterator->first->parentNode);
        nodesInStack.insert(nodeIterator->first->parentNode);

        while (!nodesToCheck.empty())
        {
            currentNode = nodesToCheck.top();
            nodesToCheck.pop();
            nodesInStack.erase(currentNode);

            // CHECK IF IT HAS NODE STATE
            auto nodeStateIterator = nodeStates.find(currentNode);

            // NO STATE FOUND -> UNPROCESSED
            if (nodeStateIterator == nodeStates.end())
            {
                // IF LEAF CHECK AND ADD STATE, ADD PARENT IF NOT ALREADY PRESENT
                if (currentNode->IsLeaf())
                {
                    if (currentNode->elements.size() <= currentNode->elementsCapacity)
                        nodeStates.insert({currentNode, std::make_pair(true, (int)currentNode->elements.size())});
                    else nodeStates.insert({currentNode, std::make_pair(false, (int)currentNode->elements.size())});

                    if (currentNode->parentNode && nodesInStack.find(currentNode->parentNode) == nodesInStack.end())
                    {
                        nodesInStack.insert(currentNode->parentNode);
                        nodesToCheck.push(currentNode->parentNode);
                    }
                }
                // IF NOT LEAF CHECK ALL CHILDS, IF ONE NOT MERGEABLE, BREAK LOOP, IF SO KEEP GOING
                // IF ONE CHILD HAS NO STATE ADD PARENT BACK TO STACK AND THEN CHILD TO BE PROCESSED
                else
                {
                    bool allChildsValid = true;
                    int childElements   = 0;

                    for (int i = 0; i < 8; ++i)
                    {
                        auto childStateIterator = nodeStates.find(currentNode->children[i]);

                        // CHILD IS PROCESSED
                        if (childStateIterator != nodeStates.end())
                        {
                            if (!childStateIterator->second.first) return removed;
                            childElements += childStateIterator->second.second;
                        }
                        // CHILD NOT PROCESSED, ADD FIRST CURRENT NODE (PARENT) TO GO BACK TO IT WHEN CHILDS PROCESSED
                        // (ONLY ONCE)
                        else
                        {
                            if (nodesInStack.find(currentNode) == nodesInStack.end())
                            {
                                nodesInStack.insert(currentNode);
                                nodesToCheck.push(currentNode);
                            }
                            if (nodesInStack.find(currentNode->children[i]) == nodesInStack.end())
                            {
                                nodesInStack.insert(currentNode->children[i]);
                                nodesToCheck.push(currentNode->children[i]);
                            }

                            allChildsValid = false;
                        }
                    }

                    if (allChildsValid && childElements <= currentNode->elementsCapacity)
                    {
                        // MERGE!!!!!! -> FIRST ADD ALL CHILD ELEMENTS TO CURRENT NODE, THEN DELETE CHILD, AND CHILDREN
                        // = NULLPTR

                        for (DynamicOctreeNode*& childNode : currentNode->children)
                        {
                            currentNode->elements.insert(
                                currentNode->elements.begin(), childNode->elements.begin(), childNode->elements.end()
                            );

                            delete childNode;

                            childNode = nullptr;
                        }

                        for (auto& element : currentNode->elements)
                            element.gameObject->SetDynamicNode(currentNode);

                        totalLeaf -= 7;

                        // DONT FORGET TO ADD IT TO NODE STATES!!!!!!
                        nodeStates.insert({currentNode, std::make_pair(true, (int)childElements)});

                        if (currentNode != rootNode && nodesInStack.find(currentNode->parentNode) == nodesInStack.end())
                        {
                            nodesInStack.insert(currentNode->parentNode);
                            nodesToCheck.push(currentNode->parentNode);
                        }
                    }
                    else if (allChildsValid && childElements > currentNode->elementsCapacity) return removed;
                }
            }
            else
            {
                // STILL MERGEABLE -> ADD PARENT TO CONTINUE ITERATION, IF NOT ROOT NODE
                if (nodeStateIterator->second.first)
                {
                    if (currentNode != rootNode && nodesInStack.find(currentNode->parentNode) == nodesInStack.end())
                    {
                        nodesInStack.insert(currentNode->parentNode);
                        nodesToCheck.push(currentNode->parentNode);
                    }
                }
                else return removed;
            }
        }
    }

    return removed;
}

const std::vector<LineSegment>& DynamicOctree::GetDrawLines()
{
    int totalLines = totalLeaf * 12;
    if (drawLines.size() == totalLines) return drawLines;

    drawLines = std::vector<LineSegment>(totalLines, LineSegment());

    std::stack<const DynamicOctreeNode*> nodesToVisit;
    nodesToVisit.push(rootNode);

    int currentDrawLine = 0;

    while (!nodesToVisit.empty())
    {
        const DynamicOctreeNode* currentNode = nodesToVisit.top();
        nodesToVisit.pop();

        if (currentNode->IsLeaf())
        {
            for (int i = 0; i < 12; ++i)
            {
                drawLines[currentDrawLine++] = currentNode->currentArea.Edge(i);
            }
        }
        else
        {
            for (DynamicOctreeNode* child : currentNode->children)
                nodesToVisit.push(child);
        }
    }
    return drawLines;
}

void DynamicOctree::UpdateTree(std::set<GameObject*> movedGameObjects)
{
    // CHECK IF GO HAS MOVED OUT OF ITS CONTAINER NODE AND REMOVE

    std::vector<GameObject*> gameObjectsToAdd;
    gameObjectsToAdd.reserve(movedGameObjects.size());

    for (GameObject* gameObject : movedGameObjects)
    {
        DynamicOctreeNode* node = gameObject->GetDynamicNode();
        AABB objectBB           = gameObject->GetGlobalAABB();

        if (gameObject->IsStatic() || !objectBB.IsFinite() || objectBB.IsDegenerate() || objectBB.Size().IsZero())
            continue;

        if (node && !node->Intersects(objectBB))
        {
            if (RemoveElement(gameObject, true))
            {
                gameObjectsToAdd.push_back(gameObject);
            }
        }
    }

    for (GameObject* gameObject : gameObjectsToAdd)
    {
        InsertElement(gameObject);
    }
}
