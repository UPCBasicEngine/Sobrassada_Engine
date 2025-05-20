#include "BaseAddon.h"

BaseAddon::BaseAddon(ResourceEmitter* owner) : ParticleAddon(owner, ParticleAddonType::BASE)
{
}

BaseAddon::BaseAddon(const rapidjson::Value& initialState, ResourceEmitter* owner) : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("Duration")) duration = initialState["Duration"].GetFloat();
    if (initialState.HasMember("Loop")) loop = initialState["Loop"].GetBool();
    if (initialState.HasMember("MaxParticles")) maxParticles = initialState["MaxParticles"].GetUint();
}

BaseAddon::~BaseAddon()
{
}

void BaseAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("Duration", duration, allocator);
    targetState.AddMember("Loop", loop, allocator);
    targetState.AddMember("MaxParticles", maxParticles, allocator);
}

void BaseAddon::Update(float deltaTime) const
{
    if (!IsEnabled()) return;

    // ADD PARTICLES IF EMITTER PARTICLES < MAX_PARTICLES
}

void BaseAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;

    // RENDER IMGUI TO CHANGE PARAMETERS
}
