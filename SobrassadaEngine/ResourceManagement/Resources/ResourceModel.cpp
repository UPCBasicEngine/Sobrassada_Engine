#include "ResourceModel.h"

ResourceModel::ResourceModel(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Mesh)
{
}

ResourceModel::~ResourceModel()
{
}
