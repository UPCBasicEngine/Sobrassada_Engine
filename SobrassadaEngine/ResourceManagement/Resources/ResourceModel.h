#pragma once

#include "Resource.h"
#include "../Utils/Transform.h"

struct Node
{
    std::string name;
    Transform transform;
    int parentIndex;
    std::vector<std::pair<UID, UID>> meshes;
};

class ResourceModel : Resource
{
  public:
    ResourceModel(UID uid, const std::string& name, const float3& maxPos, const float3& minPos);
    ~ResourceModel() override;

    void LoadData(const std::string &name, const Transform &transform, const int parentIndex, const std::vector<std::pair<UID, UID>> &meshes);

    bool HasMesh() const { return nodes.meshes.size() > 0; }

   private:
    Node nodes;
};