#include "Resource.h"


Resource::Resource(UID uid, const std::string& name, ResourceType type): uid(uid), name(name), type(type)
{
}

Resource::~Resource()
{
}