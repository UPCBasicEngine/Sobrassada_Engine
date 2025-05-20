#pragma once

#include "Resource.h"
#include "ParticleUtils.h"
#include "HashString.h"

#include "rapidjson/document.h"
#include <tuple>

class BaseAddon;
class VelocityAddon;


class ResourceEmitter : Resource
{
  public:
    ResourceEmitter(UID uid, const std::string& name);
    ResourceEmitter(UID uid, const std::string& name, const rapidjson::Value& importOptions);
    ~ResourceEmitter() override;

  private:
    HashString emitterTag;
    std::tuple<ADDON_TYPES> addonTuple = std::make_tuple(ADDON_NULLPTR);
};
