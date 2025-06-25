#pragma once

#include "HashString.h"
#include "Module.h"

#include "rapidjson/document.h"
#include <map>
#include <utility>
#include <vector>

class ParticleEmitter;
class ParticleSystem;
class ParticleSystemComponent;

class ParticleSystemModule : public Module
{
  public:
    ParticleSystemModule();
    ~ParticleSystemModule() override;

    bool Init() override;
    bool ShutDown() override;

    void RenderParticles();

    void ResquestParticleSystem(
        const HashString& requestedTag, const rapidjson::Value& initialState, ParticleSystemComponent* component
    );
    void ResquestParticleSystem(const HashString& requestedTag, ParticleSystemComponent* component);

    void DuplicateParticleSystem(
        const HashString& requestedTag, ParticleSystemComponent* component, const HashString& duplicateTag
    );

    void StopAllParticles();
    void ClearParticleSystems();

     const std::vector<HashString>& GetTags() const { return particleTags; }

  private:
    unsigned int quadVBO = 0;

    std::vector<HashString> particleTags;
    std::map<HashString, ParticleSystem*> particleSystems;

    const HashString emptyString = HashString("");
};
