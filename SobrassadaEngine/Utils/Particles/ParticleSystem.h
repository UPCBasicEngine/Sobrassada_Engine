#pragma once

#include "HashString.h"

#include "rapidjson/document.h"
#include <list>
#include <utility>
#include <vector>

namespace math
{
    class float4x4;
    class float3;
} // namespace math

class ParticleSystemComponent;
class ParticleEmitter;

class ParticleSystem
{
  public:
    ParticleSystem(const HashString& newTag, ParticleSystemComponent* component, unsigned int quadVBO);
    ParticleSystem(
        const HashString& newTag, ParticleSystemComponent* component, unsigned int quadVBO, ParticleSystem* reference
    );
    ParticleSystem(const rapidjson::Value& initialState, ParticleSystemComponent* component, unsigned int quadVBO);
    ~ParticleSystem();

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;

    void RenderParticles(const math::float4x4& VP, const math::float3& rightVector, const math::float3& upVector);

    void AddEmitter(const std::string& newEmitterName);
    void RemoveEmitter(const HashString& newEmitterTag);

    void AddComponent(ParticleSystemComponent* component);
    void RemoveComponent(std::list<ParticleSystemComponent*>::iterator componentIterator);

    void SortEmitters();
    void Stop();

    const HashString& GetTag() const { return particleSystemTag; }

  private:
    void UpdateComponents();

    // ONLY USE WHEN COPYING ANOTHER PS WHICH MEANS ITS CREATED FROM SCRATCH AND NO EMITTERS ARE PRESENT IN THIS PS
    void DuplicateEmitter(ParticleEmitter* reference);

  private:
    // quadVBO deleted from ParticleSystemModule which holds the OG.
    unsigned int quadVBO         = 0;
    HashString particleSystemTag = HashString("");
    const HashString emptyString = HashString("");

    std::vector<std::pair<HashString, ParticleEmitter*>> emitters;
    std::list<ParticleSystemComponent*> linkedComponents;
};
