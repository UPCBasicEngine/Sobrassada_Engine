#include "ColorAddon.h"

#include "EmitterInstance.h"
#include "ParticleEmitter.h"

#include "imgui.h"
#include "imgui_color_gradient.h"

static ImGradientMark* draggingMark = nullptr;
static ImGradientMark* selectedMark = nullptr;

ColorAddon::ColorAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::COLOR, owner)
{
    gradient = new ImGradient();
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
    gradient = new ImGradient();
}

ColorAddon::~ColorAddon()
{
    delete gradient;
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
        gradient->getColorAt(0, &particle.color[0]);
    }
}

void ColorAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    for (auto& particle : emitterInstance->particles)
    {
        float colorPosition = particle.currentLifetime / particle.lifeTime;
        if (colorPosition > 1.f) colorPosition = 1.f;

        gradient->getColorAt(colorPosition, &particle.color[0]);

        int x = 0;
    }
}

void ColorAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Color Addon");
    ImGui::PushItemWidth(200);

    ImGui::ColorEdit4("Particle color", &particleColor[0]);

    if (ImGui::GradientEditor(gradient, draggingMark, selectedMark))
    {
        gradient->getColorAt(0.f, &particleColor[0]);
    };

    ImGui::PopItemWidth();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
