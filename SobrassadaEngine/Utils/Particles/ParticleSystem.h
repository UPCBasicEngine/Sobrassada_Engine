#pragma once

#include "HashString.h"

#include "rapidjson/document.h"
#include <list>
#include <utility>
#include <vector>

class ParticleSystemComponent;
class ParticleEmitter;

class ParticleSystem
{
  public:
    ParticleSystem(const HashString& newTag, ParticleSystemComponent* component);
    ParticleSystem(const rapidjson::Value& initialState, ParticleSystemComponent* component);
    ~ParticleSystem();

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void RenderParticles();

    void AddEmitter(const std::string& newEmitterName);
    void RemoveEmitter(const HashString& newEmitterTag);

    void AddComponent(ParticleSystemComponent* component);
    void RemoveComponent(std::list<ParticleSystemComponent*>::iterator componentIterator);

    const HashString& GetTag() const { return particleSystemTag; }

  private:
    void UpdateComponents();

  private:
    HashString particleSystemTag = HashString("");
    const HashString emptyString       = HashString("");

    std::vector<std::pair<HashString, ParticleEmitter*>> emitters;
    std::list<ParticleSystemComponent*> linkedComponents;
};
