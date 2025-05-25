#include "VelocityAddon.h"

#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

#include "imgui.h"

VelocityAddon::VelocityAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::VELOCITY, owner)
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

void VelocityAddon::Init(EmitterInstance* emitterInstance)
{
    for (auto& particle : emitterInstance->particles)
    {
        particle.velocity = float3(rng->Float(-startSpeed, startSpeed), rng->Float(-startSpeed, startSpeed), 0.f);
    }
}

void VelocityAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (!IsEnabled()) return;

    for (auto& particle : emitterInstance->particles)
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
