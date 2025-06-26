#pragma once

#include "Component.h"
#include "HashString.h"

#include <utility>
#include <vector>

class ParticleEmitter;
class EmitterInstance;
class ParticleSystem;

class ParticleSystemComponent : public Component
{
  public:
    ParticleSystemComponent(UID uid, GameObject* parent);
    ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~ParticleSystemComponent() override;

    void Init() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;

    void ReloadEmitterInstances(const std::vector<std::pair<HashString, ParticleEmitter*>>& emitters);
    void StopInstances();

    const HashString& GetParticleSystemTag() const { return particleSystemTag; };

    void SetParticleSystem(ParticleSystem* newParticleSystem);
    void SetParticleIterator(std::list<ParticleSystemComponent*>::iterator iterator)
    {
        particleSystemIterator = iterator;
    }

  private:
    void CreateLocalAABB();
  private:
    char newParticleTagName[64]     = "";
    char newEmitterTagName[64]      = "";
    HashString particleSystemTag    = HashString("");

    ParticleSystem* particleSystem  = nullptr;
    EmitterInstance* currentEmitter = nullptr;

    std::vector<EmitterInstance> emitterInstances;
    std::list<ParticleSystemComponent*>::iterator particleSystemIterator;
};
