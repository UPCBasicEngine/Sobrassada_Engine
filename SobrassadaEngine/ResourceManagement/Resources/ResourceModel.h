#pragma once

#include "Resource.h"
#include "FileSystem/Model.h"
#include "Utils/Transform.h"

class ResourceModel : Resource
{
  public:
    ResourceModel(UID uid, const std::string& name);
    ~ResourceModel() override;

    void LoadData(const std::string &name, const Transform &transform, const int parentIndex, const std::vector<std::pair<UID, UID>> &meshes);

   private:
    Model modelData;
};