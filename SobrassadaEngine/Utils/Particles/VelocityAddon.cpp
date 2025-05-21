#include "VelocityAddon.h"

#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

#include "imgui.h"

VelocityAddon::VelocityAddon(ParticleEmitter* owner) : ParticleAddon(owner, ParticleAddonType::VELOCITY)
{
}

VelocityAddon::VelocityAddon(const rapidjson::Value& initialState, ParticleEmitter* owner)
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

    rapidjson::Value centerOffsetSave(rapidjson::kArrayType);
    targetState.AddMember("StartSpeed", startSpeed, allocator);
}

void VelocityAddon::Init()
{
    // ADD INITIAL Y VELOCITY TO PARTICLES

    for (auto& particle : emitterOwner->particles)
    {
        particle.velocity = float3(rng->Float(-startSpeed, startSpeed), startSpeed, 0.f);
    }
}

void VelocityAddon::Update(float deltaTime)
{
    if (!IsEnabled()) return;

    // MODIFY PARTICLE POSITION DEPENDING ON VELOCITY
    for (auto& particle : emitterOwner->particles)
    {
        particle.position = particle.position.Mul(particle.velocity*deltaTime);
    }
}

void VelocityAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;
    // RENDER IMGUI TO CHANGE PARAMETERS
    ImGui::Text("VELOCITY ADDON");

    ImGui::DragFloat("Start velocity", &startSpeed);

    ImGui::Separator();
}
