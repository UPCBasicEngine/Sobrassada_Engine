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
    gradient = new ImGradient();
    gradient->getMarks().clear();

    if (initialState.HasMember("ColorMarks") && initialState["ColorMarks"].IsArray())
    {
        const rapidjson::Value& colorMarkArray = initialState["ColorMarks"];

        for (rapidjson::SizeType i = 0; i < colorMarkArray.Size(); i++)
        {
            const rapidjson::Value& currentMark = colorMarkArray[i];
            gradient->addMark(
                currentMark[0].GetFloat(), ImColor(
                                               currentMark[1].GetFloat(), currentMark[2].GetFloat(),
                                               currentMark[3].GetFloat(), currentMark[4].GetFloat()
                                           )
            );
        }
    }
}

ColorAddon::~ColorAddon()
{
    delete gradient;
    gradient = nullptr;
}

void ColorAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    auto& gradientMarks = gradient->getMarks();
    rapidjson::Value colorMarksSave(rapidjson::kArrayType);

    for (ImGradientMark* mark : gradientMarks)
    {
        rapidjson::Value currentMarksSave(rapidjson::kArrayType);
        currentMarksSave.PushBack(mark->position, allocator)
            .PushBack(mark->color[0], allocator)
            .PushBack(mark->color[1], allocator)
            .PushBack(mark->color[2], allocator)
            .PushBack(mark->color[3], allocator);

        colorMarksSave.PushBack(currentMarksSave, allocator);
    }

    targetState.AddMember("ColorMarks", colorMarksSave, allocator);
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
    for (int i = 0; i <= emitterInstance->particleVectorPos; ++i)
    {
        Particle& particle  = emitterInstance->particles[i];

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

    ImGui::GradientEditor(gradient, draggingMark, selectedMark);

    ImGui::PopItemWidth();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void ColorAddon::Duplicate(ParticleAddon* reference)
{
    ColorAddon* other = reinterpret_cast<ColorAddon*>(reference);

    if (other)
    {
        particleColor = other->particleColor;

        gradient->getMarks().clear();

        for (auto mark : other->gradient->getMarks())
            gradient->addMark(mark->position, ImColor(mark->color[0], mark->color[1], mark->color[2], mark->color[3]));
    }
}
