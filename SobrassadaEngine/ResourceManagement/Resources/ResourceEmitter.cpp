#include "ResourceEmitter.h"

#include "BaseAddon.h"
#include "VelocityAddon.h"

ResourceEmitter::ResourceEmitter(UID uid, const std::string& name) : Resource(uid, name, ResourceType::ParticleEmmiter)
{
}

ResourceEmitter::ResourceEmitter(UID uid, const std::string& name, const rapidjson::Value& importOptions)
    : Resource(uid, name, ResourceType::ParticleEmmiter)
{
}

ResourceEmitter::~ResourceEmitter()
{
    std::apply([](auto&... tupleVar) { ((delete tupleVar, tupleVar = nullptr), ...); }, addonTuple);
}
