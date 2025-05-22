#include "EmitterInstance.h"

#include "ParticleEmitter.h"

EmitterInstance::~EmitterInstance()
{
}


void EmitterInstance::Update(float deltaTime)
{
    emitter->Update()
}

void EmitterInstance::RenderEditor()
{

}