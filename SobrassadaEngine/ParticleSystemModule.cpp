#include "ParticleSystemModule.h"

#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

ParticleSystemModule::ParticleSystemModule()
{
}

ParticleSystemModule::~ParticleSystemModule()
{
}

bool ParticleSystemModule::Init()
{
    return true;
}

bool ParticleSystemModule::ShutDown()
{
    for (auto& pair : particleEmitters)
    {
        delete pair.second;
    }

    return true;
}

void ParticleSystemModule::RenderParticles()
{
}

ParticleEmitter* ParticleSystemModule::RequestParticleEmitter(const std::string& name, ParticleSystemComponent* owner)
{
    ParticleEmitter* emitter = new ParticleEmitter(GenerateUID(), name, owner);
    particleEmitters.insert({emitter->GetUID(), emitter});
    return emitter;
}

ParticleEmitter*
ParticleSystemModule::RequestParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner)
{
    ParticleEmitter* emitter = new ParticleEmitter(initialState, owner);
    particleEmitters.insert({emitter->GetUID(), emitter});
    return emitter;
}

void ParticleSystemModule::DeleteParticleEmitter(UID emiterUID)
{
    auto iterator = particleEmitters.find(emiterUID);
    if (iterator != particleEmitters.end())
    {
        delete iterator->second;
        particleEmitters.erase(iterator);
    }
}
