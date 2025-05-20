#include "ParticleSystemComponent.h"

ParticleSystemComponent::ParticleSystemComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "ParticleSystem", COMPONENT_PARTICLE_SYSTEM)
{
}

ParticleSystemComponent::ParticleSystemComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
}

ParticleSystemComponent::~ParticleSystemComponent()
{
}

void ParticleSystemComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
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
}
