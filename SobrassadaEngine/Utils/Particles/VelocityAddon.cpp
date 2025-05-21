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
        particle.velocity = float3(rng->Float(-startSpeed, startSpeed), rng->Float(-startSpeed, startSpeed), 0.f);
    }
}

void VelocityAddon::Update(float deltaTime)
{
    if (!IsEnabled()) return;

    for (auto& particle : emitterOwner->particles)
    {
        particle.position = particle.position.Add(particle.velocity*deltaTime);
    }
}

void VelocityAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;

    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Velocity Addon");

    ImGui::PushItemWidth(100);

    ImGui::DragFloat("Start velocity", &startSpeed);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
