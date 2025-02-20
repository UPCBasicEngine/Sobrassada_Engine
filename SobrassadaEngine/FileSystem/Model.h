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

    void SetUID(const UID uid) { this->uid = uid; }
    void SetNodes(const std::vector<NodeData> &nodes) { this->nodes = nodes; }



  private:
    UID uid;
    std::vector<NodeData> nodes;
};