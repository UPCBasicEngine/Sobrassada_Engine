#pragma once

#include "HashString.h"

#include "rapidjson/document.h"
#include <vector>
#include <utility>

class ParticleSystemComponent;
class ParticleEmitter;

class ParticleSystem
{
  public:
    ParticleSystem();
    ParticleSystem(const rapidjson::Value& initialState, ParticleSystemComponent* owner);
    ~ParticleSystem();
    
  private:
    HashString particleSystemTag       = HashString("");
    std::vector<std::pair<HashString, ParticleEmitter*>> emitters;
};
