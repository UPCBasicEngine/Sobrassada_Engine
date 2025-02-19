#pragma once

#include "Globals.h"
#include "Utils/Transform.h"

#include <vector>


struct NodeData
{
    std::string name;
    Transform transform;
    int parentId;
    std::vector<std::pair<UID, UID>> meshes;
};

class Model
{
  public:
    Model() = default;

    void AddNode(const NodeData&& newNode) { nodes.emplace_back(newNode); }



  private:
    std::vector<NodeData> nodes;
};