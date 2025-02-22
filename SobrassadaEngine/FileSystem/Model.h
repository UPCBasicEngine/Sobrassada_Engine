#pragma once

#include "Globals.h"
#include "Utils/Transform.h"

#include <vector>

struct NodeData
{
    std::string name;
    Transform transform;
    int parentIndex;
    std::vector<std::pair<UID, UID>> meshes;
};

class Model
{
  public:
    Model() = default;
    Model(const UID id, const std::vector<NodeData>& nodes) : uid(id), nodes(nodes) {};

    const std::vector<NodeData>& GetNodes() const { return nodes; }
    size_t GetNodesCount() const { return nodes.size(); }

    void SetUID(const UID uid) { this->uid = uid; }
    void SetNodes(const std::vector<NodeData>& nodes) { this->nodes = nodes; }

  private:
    UID uid;
    std::vector<NodeData> nodes;
};