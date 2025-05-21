#include "ParticleSystemModule.h"

#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

#include "glew.h"

ParticleSystemModule::ParticleSystemModule()
{
}

ParticleSystemModule::~ParticleSystemModule()
{
}

bool ParticleSystemModule::Init()
{
    float vertexData[] = {
        -0.5, 0.5,  0.f, //
        -0.5, -0.5, 0.f, //
        0.5,  -0.5, 0.f, //

        -0.5, 0.5,  0.f, //
        0.5,  -0.5, 0.f, //
        0.5,  0.5,  0.f, //

        0.f,  1.f, //
        0.f,  0.f, //
        1.f,  0.f, //

        0.f,  1.f, //
        1.f,  0.f, //
        1.f,  1.f, //
    };

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool ParticleSystemModule::ShutDown()
{
    for (auto& pair : particleEmitters)
    {
        delete pair.second;
    }

    glDeleteBuffers(1, &quadVBO);

    return true;
}

void ParticleSystemModule::RenderParticles()
{
}

ParticleEmitter* ParticleSystemModule::RequestParticleEmitter(const std::string& name, ParticleSystemComponent* owner)
{
    ParticleEmitter* emitter = new ParticleEmitter(GenerateUID(), name, owner);
    particleEmitters.insert({emitter->GetUID(), emitter});
    emitter->SetQuadVBO(quadVBO);
    return emitter;
}

ParticleEmitter*
ParticleSystemModule::RequestParticleEmitter(const rapidjson::Value& initialState, ParticleSystemComponent* owner)
{
    ParticleEmitter* emitter = new ParticleEmitter(initialState, owner);
    particleEmitters.insert({emitter->GetUID(), emitter});
    emitter->SetQuadVBO(quadVBO);
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
