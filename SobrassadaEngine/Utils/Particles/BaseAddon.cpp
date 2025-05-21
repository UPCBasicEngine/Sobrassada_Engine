#include "BaseAddon.h"

#include "GameObject.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

#include "imgui.h"

BaseAddon::BaseAddon(ParticleEmitter* owner) : ParticleAddon(owner, ParticleAddonType::BASE)
{
}

BaseAddon::BaseAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("Duration")) duration = initialState["Duration"].GetFloat();
    if (initialState.HasMember("Loop")) loop = initialState["Loop"].GetBool();
    if (initialState.HasMember("MaxParticles")) maxParticles = initialState["MaxParticles"].GetInt();

    if (initialState.HasMember("RandomLifetime")) randomLifetime = initialState["RandomLifetime"].GetBool();
    if (initialState.HasMember("MinLifetime")) minLifetime = initialState["MinLifetime"].GetFloat();
    if (initialState.HasMember("MaxLifetime")) maxLifetime = initialState["MaxLifetime"].GetFloat();
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

    targetState.AddMember("RandomLifetime", randomLifetime, allocator);
    targetState.AddMember("MinLifetime", minLifetime, allocator);
    targetState.AddMember("MaxLifetime", maxLifetime, allocator);
}

void BaseAddon::Init()
{
    emitterOwner->particles.clear();
    emitterOwner->particles.reserve(maxParticles);

    const float3 startingPosition = emitterOwner->GetOwner()->GetGlobalTransform().TranslatePart();
    emitterOwner->particles.assign(maxParticles, Particle(startingPosition));

    for (auto& particle : emitterOwner->particles)
    {
        particle.lifeTime = randomLifetime ? rng->Float(minLifetime, maxLifetime) : maxLifetime;
    }

    currentEmissionTime    = 0.f;
    emitterOwner->isEmitting = true;
}

void BaseAddon::Update(float deltaTime)
{
    if (emitterOwner->isEmitting)
    {
        float3 emitterPosition = emitterOwner->GetOwner()->GetGlobalTransform().TranslatePart();

        for (auto& particle : emitterOwner->particles)
        {
            if (particle.alive)
            {
                particle.lifeTime -= deltaTime;
                if (particle.lifeTime <= 0)
                {
                    particle.lifeTime = 0.f;
                    particle.alive    = false;
                }
            }
            else
            {
                particle.alive    = true;
                particle.lifeTime = randomLifetime ? rng->Float(minLifetime, maxLifetime) : maxLifetime;
                particle.position = emitterPosition;
            }
        }

        currentEmissionTime += deltaTime;
    }
    
    if (currentEmissionTime > duration)
    {
        emitterOwner->isEmitting = false;
    }
}

void BaseAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Base Addon");

    ImGui::PushItemWidth(100);

    ImGui::Checkbox("Loop", &loop);
    ImGui::InputFloat("Duration", &duration, 0.05f, 1.f);
    ImGui::InputInt("Max Particles", &maxParticles, 5, 10);

    if (randomLifetime)
    {
        ImGui::InputFloat("##MinLifetime", &minLifetime);
        ImGui::SameLine();
    }
    ImGui::InputFloat("##MaxLifetime", &maxLifetime);
    ImGui::SameLine();
    ImGui::Text("Lifetime");
    ImGui::SameLine();
    ImGui::Checkbox("Randomize", &randomLifetime);

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
