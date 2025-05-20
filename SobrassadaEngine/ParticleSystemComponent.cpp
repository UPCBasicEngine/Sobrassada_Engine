#include "ParticleSystemComponent.h"

#include "Application.h"
#include "ParticleEmitter.h"
#include "ParticleSystemModule.h"

ParticleSystemComponent::ParticleSystemComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "ParticleSystem", COMPONENT_PARTICLE_SYSTEM)
{
    emitter = App->GetParticleModule()->RequestParticleEmitter(this);
}

ParticleSystemComponent::ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    emitter = App->GetParticleModule()->RequestParticleEmitter(initialState, this);
}


ParticleSystemComponent::~ParticleSystemComponent()
{
    if (emitter) App->GetParticleModule()->DeleteParticleEmitter(emitter->GetUID());
}

void ParticleSystemComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
    if (emitter) emitter->Save(targetState, allocator);
}

void ParticleSystemComponent::Clone(const Component* other)
{
}

void ParticleSystemComponent::Update(float deltaTime)
{
}

void ParticleSystemComponent::Render(float deltaTime)
{
}

void ParticleSystemComponent::RenderDebug(float deltaTime)
{
}

void ParticleSystemComponent::RenderEditorInspector()
{
    if (emitter) emitter->RenderEditor();
}
