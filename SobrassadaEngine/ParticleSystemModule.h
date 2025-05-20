#pragma once

#include "Module.h"

#include "rapidjson/document.h"
#include <map>

class ParticleEmitter;
class ParticleSystemComponent;

class ParticleSystemModule : public Module
{
  public:
    ParticleSystemModule();
    ~ParticleSystemModule() override;

    bool Init() override;
    bool ShutDown() override;

    void RenderParticles();

    ParticleEmitter* RequestParticleEmitter(const std::string& name, ParticleSystemComponent* owner);
    ParticleEmitter* RequestParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner);

    void DeleteParticleEmitter(UID emiterUID);

  private:
    std::map<UID, ParticleEmitter*> particleEmitters;
};
