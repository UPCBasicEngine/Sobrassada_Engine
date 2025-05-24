#include "EmitterInstance.h"

#include "ParticleEmitter.h"

#include "imgui.h"

EmitterInstance::~EmitterInstance()
{
}

const std::string& EmitterInstance::GetName() const
{
    if (emitter) return emitter->GetName();
}

const HashString& EmitterInstance::GetTag() const
{
    if (emitter) return emitter->GetTag();
}

void EmitterInstance::Update(float deltaTime)
{
    // Change ParticleEmitter and ParticleAddon to recieve the EmitterInstance and update its particles
    emitter->Update(deltaTime, this);
}

void EmitterInstance::RenderEditor()
{
    if (ImGui::Button("Spawn Particles")) emitter->Spawn(this);

    emitter->RenderEditor();
}