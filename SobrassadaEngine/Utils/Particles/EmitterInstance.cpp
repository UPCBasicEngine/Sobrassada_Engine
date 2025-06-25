#include "EmitterInstance.h"

#include "ParticleEmitter.h"

#include "imgui.h"

EmitterInstance::EmitterInstance(ParticleEmitter* newEmitter, ParticleSystemComponent* newOwner)
    : emitter(newEmitter), owner(newOwner)
{
    emitterTag = newEmitter->GetTag();
}

EmitterInstance::~EmitterInstance()
{
}

void EmitterInstance::Spawn()
{
    emitter->Spawn(this);
}

void EmitterInstance::Stop()
{
    isEmitting          = false;
    currentEmissionTime = 0.f;
    particleVectorPos   = 0;
}

void EmitterInstance::Update(float deltaTime)
{
    emitter->Update(deltaTime, this);
}

void EmitterInstance::RenderEditor()
{
    if (ImGui::Button("Spawn Particles")) emitter->Spawn(this);

    emitter->RenderEditor();
}

void EmitterInstance::RenderDebug(GameObject* parent)
{
    if (emitter) emitter->RenderDebug(parent);
}
