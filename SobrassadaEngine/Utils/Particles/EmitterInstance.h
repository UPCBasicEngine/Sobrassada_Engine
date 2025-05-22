#pragma once

#include "Particle.h"

#include <vector>

class ParticleSystemComponent;
class ParticleEmitter;

class EmitterInstance
{
  public:
    EmitterInstance(ParticleEmitter* newEmitter, ParticleSystemComponent* newOwner)
        : emitter(newEmitter), owner(newOwner) {};
    ~EmitterInstance();

    void Update(float deltaTime);
    void RenderEditor();

    unsigned int aliveParticles = 0;
    std::vector<Particle> particles;

  private:
    ParticleEmitter* emitter       = nullptr;
    ParticleSystemComponent* owner = nullptr;
};
