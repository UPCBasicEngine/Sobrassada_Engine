#include "EmitterInstance.h"

#include "ParticleEmitter.h"

EmitterInstance::~EmitterInstance()
{
}

void EmitterInstance::Update(float deltaTime)
{
    // Change ParticleEmitter and ParticleAddon to recieve the EmitterInstance and update its particles
    emitter->Update(deltaTime, this);
}

void EmitterInstance::RenderEditor()
{
    emitter->RenderEditor();
}