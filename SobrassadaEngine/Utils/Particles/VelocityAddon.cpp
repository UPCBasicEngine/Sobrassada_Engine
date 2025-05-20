#include "VelocityAddon.h"

VelocityAddon::VelocityAddon(ResourceEmitter* owner) : ParticleAddon(owner, ParticleAddonType::VELOCITY)
{
}

VelocityAddon::VelocityAddon(const rapidjson::Value& initialState, ResourceEmitter* owner)
    : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("StartSpeed")) startSpeed = initialState["StartSpeed"].GetFloat();
}

VelocityAddon::~VelocityAddon()
{
}

void VelocityAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("StartSpeed", startSpeed, allocator);
}

void VelocityAddon::Init()
{
    // ADD INITIAL Y VELOCITY TO PARTICLES
}

void VelocityAddon::Update(float deltaTime) const
{
    if (!IsEnabled()) return;

    // MODIFY PARTICLE POSITION DEPENDING ON VELOCITY
}

void VelocityAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;

    // RENDER IMGUI TO CHANGE PARAMETERS
}
