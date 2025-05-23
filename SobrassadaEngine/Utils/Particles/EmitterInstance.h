#pragma once

#include "Particle.h"

#include "HashString.h"

#include <vector>
#include <string>

class ParticleSystemComponent;
class ParticleEmitter;

class EmitterInstance
{
  public:
    EmitterInstance(ParticleEmitter* newEmitter, ParticleSystemComponent* newOwner)
        : emitter(newEmitter), owner(newOwner) {};
    ~EmitterInstance();

    const std::string& GetName() const;
    const HashString& GetTag() const;

    void Update(float deltaTime);
    void RenderEditor();

    ParticleSystemComponent* GetOwner() { return owner; }

    unsigned int aliveParticles = 0;
    std::vector<Particle> particles;

    float currentEmissionTime = 0.f;
    bool isEmitting           = false;

  private:
    ParticleEmitter* emitter       = nullptr;
    ParticleSystemComponent* owner = nullptr;
};
