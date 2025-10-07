#include "ParticleSystem.h"

#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

#include "Math/float4x4.h"
#include <algorithm>

ParticleSystem::ParticleSystem(const HashString& newTag, ParticleSystemComponent* component, unsigned int quadVBO)
    : particleSystemTag(newTag), quadVBO(quadVBO)
{
    auto componentIterator = linkedComponents.insert(linkedComponents.end(), component);
    component->SetParticleIterator(componentIterator);
    component->SetParticleSystem(this);
}

ParticleSystem::ParticleSystem(
    const HashString& newTag, ParticleSystemComponent* component, unsigned int quadVBO, ParticleSystem* reference
)
    : particleSystemTag(newTag), quadVBO(quadVBO)
{

    for (auto& emitterPair : reference->emitters)
    {
        DuplicateEmitter(emitterPair.second);
    }

    auto componentIterator = linkedComponents.insert(linkedComponents.end(), component);
    component->SetParticleIterator(componentIterator);
    component->SetParticleSystem(this);

    UpdateComponents();

    UpdateComponentsAABB();

    SortEmitters();
}

ParticleSystem::ParticleSystem(
    const rapidjson::Value& initialState, ParticleSystemComponent* component, unsigned int quadVBO
)
    : quadVBO(quadVBO)
{
    if (initialState.HasMember("ParticleSystemTag"))
        particleSystemTag = HashString(initialState["ParticleSystemTag"].GetString());

    if (initialState.HasMember("Emitters") && initialState["Emitters"].IsArray())
    {
        const rapidjson::Value& jsonEmitters = initialState["Emitters"];

        for (rapidjson::SizeType i = 0; i < jsonEmitters.Size(); i++)
        {
            const rapidjson::Value& newEmitterJSON = jsonEmitters[i];

            ParticleEmitter* newEmitter            = new ParticleEmitter(newEmitterJSON, this);
            newEmitter->SetQuadVBO(quadVBO);
            emitters.push_back({newEmitter->GetName(), newEmitter});
        }
    }

    auto componentIterator = linkedComponents.insert(linkedComponents.end(), component);
    component->SetParticleIterator(componentIterator);
    component->SetParticleSystem(this);
    component->ReloadEmitterInstances(emitters);

    UpdateComponentsAABB();

    SortEmitters();
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    targetState.AddMember("ParticleSystemTag", rapidjson::Value(particleSystemTag.c_str(), allocator), allocator);

    rapidjson::Value emittersArrayJSON(rapidjson::kArrayType);

    for (auto& emitter : emitters)
    {
        rapidjson::Value currentEmitterJSON(rapidjson::kObjectType);
        emitter.second->Save(currentEmitterJSON, allocator);
        emittersArrayJSON.PushBack(currentEmitterJSON, allocator);
    }

    targetState.AddMember("Emitters", emittersArrayJSON, allocator);
}

void ParticleSystem::RenderParticles(const float4x4& VP, const float3& rightVector, const float3& upVector)
{
    for (auto& emitter : emitters)
    {
        emitter.second->RenderParticles(VP, rightVector, upVector);
    }
}

void ParticleSystem::AddEmitter(const std::string& newEmitterName)
{
    HashString newEmitterTag = HashString(newEmitterName);
    if (newEmitterTag == emptyString) return;

    int position = -1;
    for (int i = 0; i < emitters.size(); ++i)
    {
        if (emitters[i].first == newEmitterTag)
        {
            position = i;
            break;
        }
    }

    if (position < 0)
    {
        ParticleEmitter* newEmitter = new ParticleEmitter(newEmitterTag, this);
        newEmitter->SetQuadVBO(quadVBO);
        emitters.push_back({newEmitterTag, newEmitter});
    }

    UpdateComponents();

    UpdateComponentsAABB();

    SortEmitters();
}

// ONLY USE WHEN COPYING ANOTHER PS WHICH MEANS ITS CREATED FROM SCRATCH AND NO EMITTERS ARE PRESENT IN THIS PS
void ParticleSystem::DuplicateEmitter(ParticleEmitter* reference)
{
    ParticleEmitter* newEmitter = new ParticleEmitter(reference, this);
    newEmitter->SetQuadVBO(quadVBO);
    emitters.push_back({newEmitter->GetTag(), newEmitter});
}

void ParticleSystem::RemoveEmitter(const HashString& newEmitterTag)
{
    int position = -1;
    for (int i = 0; i < emitters.size(); ++i)
    {
        if (emitters[i].first == newEmitterTag)
        {
            position = i;
            break;
        }
    }

    if (position > -1)
    {
        emitters.erase(emitters.begin() + position);
    }

    UpdateComponents();

    UpdateComponentsAABB();

    SortEmitters();
}

void ParticleSystem::AddComponent(ParticleSystemComponent* component)
{
    auto componentIterator = linkedComponents.insert(linkedComponents.end(), component);
    component->SetParticleIterator(componentIterator);
    component->SetParticleSystem(this);
    component->ReloadEmitterInstances(emitters);
    component->UpdateAABB(minValue, maxValue);
}

void ParticleSystem::RemoveComponent(std::list<ParticleSystemComponent*>::iterator componentIterator)
{
    linkedComponents.erase(componentIterator);
}

void ParticleSystem::SortEmitters()
{
    std::sort(
        emitters.begin(), emitters.end(),
        [](const std::pair<HashString, ParticleEmitter*>& a, const std::pair<HashString, ParticleEmitter*>& b)
        { return a.second->GetRenderPriority() > b.second->GetRenderPriority(); }
    );
}

void ParticleSystem::Stop()
{
    for (auto component : linkedComponents)
    {
        component->StopInstances();
    }
}

void ParticleSystem::UpdateComponentsAABB()
{
    CalculateMinMaxValues();

    for (auto component : linkedComponents)
    {
        component->UpdateAABB(minValue, maxValue);
    }
}

void ParticleSystem::UpdateComponents()
{
    for (auto component : linkedComponents)
    {
        component->ReloadEmitterInstances(emitters);
    }
}

void ParticleSystem::CalculateMinMaxValues()
{
    ParticleValues particleValue;

    for (auto& emitter : emitters)
    {
        emitter.second->GetParticleValues(particleValue);
    }

    float maxSize      = fmax(fmax(particleValue.size.x, particleValue.size.y), particleValue.size.z);
    particleValue.size = float3(maxSize);

    float3 maxLocation = (particleValue.speed.Mul(float3::one.Normalized())) * particleValue.lifeTime;

    maxValue           = particleValue.areaOffset + maxLocation + (particleValue.size / 2.0);
    minValue           = -maxValue;
}
