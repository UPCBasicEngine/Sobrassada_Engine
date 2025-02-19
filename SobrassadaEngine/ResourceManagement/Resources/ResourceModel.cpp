#include "ResourceModel.h"

ResourceModel::ResourceModel(UID uid, const std::string& name, const float3& maxPos, const float3& minPos)
    : Resource(uid, name, ResourceType::Mesh)
{
}

ResourceModel::~ResourceModel()
{
}

