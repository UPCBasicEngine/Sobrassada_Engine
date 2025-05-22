#include "ParticleSystemModule.h"

#include "Application.h"
#include "CameraComponent.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"
#include "SceneModule.h"

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
    bool playMode                     = App->GetSceneModule()->GetInPlayMode();
    const Frustum& editorCamera       = App->GetCameraModule()->GetCamera();
    const CameraComponent* gameCamera = App->GetSceneModule()->GetScene()->GetMainCamera();

    float4x4 VP;
    float3 rightVector;
    float3 upVector;

    if (playMode && gameCamera)
    {
        VP          = gameCamera->GetProjectionMatrix() * gameCamera->GetViewMatrix();
        rightVector = gameCamera->GetCameraRight();
        upVector    = gameCamera->GetCameraUp();
    }
    else
    {
        VP          = editorCamera.ProjectionMatrix() * editorCamera.ViewMatrix();
        rightVector = editorCamera.WorldRight();
        upVector    = editorCamera.up;
    }

    for (auto& emitter : particleEmitters)
    {
        emitter.second->RenderParticles(VP, rightVector, upVector);
    }
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
