#include "ColorAddon.h"

#include "EmitterInstance.h"
#include "ParticleEmitter.h"

#include "imgui.h"

ColorAddon::ColorAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::COLOR, owner)
{
}

ColorAddon::ColorAddon(const rapidjson::Value& initialState, ParticleEmitter* owner)
    : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("ParticleColor"))
    {
        const rapidjson::Value& dataArray = initialState["ParticleColor"];
        particleColor                     = {
            dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat(), dataArray[3].GetFloat()
        };
    }
}

ColorAddon::~ColorAddon()
{
}

void ColorAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    rapidjson::Value particleColorSave(rapidjson::kArrayType);
    particleColorSave.PushBack(particleColor.x, allocator)
        .PushBack(particleColor.y, allocator)
        .PushBack(particleColor.z, allocator)
        .PushBack(particleColor.w, allocator);
    targetState.AddMember("ParticleColor", particleColorSave, allocator);
}

void ColorAddon::Init(EmitterInstance* emitterInstance)
{
    for (auto& particle : emitterInstance->particles)
    {
        particle.color = particleColor;
    }
}

void ColorAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
}

void ColorAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Color Addon");
    ImGui::PushItemWidth(200);

    ImGui::ColorEdit4("Particle color", &particleColor[0]);

    ImGui::PopItemWidth();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
