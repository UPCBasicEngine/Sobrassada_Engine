#include "ParticleSystemModule.h"

#include "Application.h"
#include "CameraComponent.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.h"
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
    constexpr float vertexData[] = {
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
    ClearParticleSystems();

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

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);

    for (auto& emitter : particleSystems)
    {
        emitter.second->RenderParticles(VP, rightVector, upVector);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void ParticleSystemModule::ResquestParticleSystem(
    const HashString& requestedTag, const rapidjson::Value& initialState, ParticleSystemComponent* component
)
{
    if (emptyString == requestedTag) return;

    auto particleSystemIterator = particleSystems.find(requestedTag);

    if (particleSystemIterator == particleSystems.end())
    {
        ParticleSystem* newPS = new ParticleSystem(initialState, component, quadVBO);

        particleSystems.insert({requestedTag, newPS});
        particleTags.push_back(requestedTag);
    }
    else
    {
        particleSystemIterator->second->AddComponent(component);
    }
}

void ParticleSystemModule::ResquestParticleSystem(const HashString& requestedTag, ParticleSystemComponent* component)
{
    if (emptyString == requestedTag) return;

    auto particleSystemIterator = particleSystems.find(requestedTag);

    if (particleSystemIterator == particleSystems.end())
    {
        ParticleSystem* newPS = new ParticleSystem(requestedTag, component, quadVBO);
        particleSystems.insert({requestedTag, newPS});
        particleTags.push_back(requestedTag);
    }
    else
    {
        particleSystemIterator->second->AddComponent(component);
    }
}

void ParticleSystemModule::DuplicateParticleSystem(
    const HashString& requestedTag, ParticleSystemComponent* component, const HashString& duplicateTag
)
{
    if (emptyString == requestedTag || emptyString == duplicateTag) return;

    auto duplicatedIterator = particleSystems.find(duplicateTag);
    auto requestedIterator  = particleSystems.find(requestedTag);

    if (requestedIterator == particleSystems.end() && duplicatedIterator != particleSystems.end())
    {
        ParticleSystem* newPS = new ParticleSystem(requestedTag, component, quadVBO, duplicatedIterator->second);
        particleSystems.insert({requestedTag, newPS});
        particleTags.push_back(requestedTag);
    }
}

void ParticleSystemModule::StopAllParticles()
{
    for (auto& pair : particleSystems)
    {
        pair.second->Stop();
    }
}

void ParticleSystemModule::ClearParticleSystems()
{
    for (auto& pair : particleSystems)
    {
        delete pair.second;
    }

    particleTags.clear();
    particleSystems.clear();
}
