#pragma once

#include "Particle.h"

#include "HashString.h"

#include <string>
#include <vector>

class ParticleSystemComponent;
class ParticleEmitter;
class GameObject;

class EmitterInstance
{
  public:
    EmitterInstance(ParticleEmitter* newEmitter, ParticleSystemComponent* newOwner)
        : emitter(newEmitter), owner(newOwner) {};
    ~EmitterInstance();

    const std::string& GetName() const;
    const HashString& GetTag() const;

    void Spawn();
    void Update(float deltaTime);
    void RenderEditor();
    void RenderDebug(GameObject* parent);

    ParticleSystemComponent* GetOwner() { return owner; }
    ParticleEmitter* GetEmitter() { return emitter; }

    unsigned int aliveParticles = 0;
    std::vector<Particle> particles;

    float currentEmissionTime = 0.f;
    bool isEmitting           = false;
    int particleVectorPos     = 0;

  private:
    ParticleEmitter* emitter       = nullptr;
    ParticleSystemComponent* owner = nullptr;
};
