#include "SpritesheetAddon.h"

#include "EmitterInstance.h"
#include "ParticleEmitter.h"

#include "imgui.h"

SpritesheetAddon::SpritesheetAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::SPRITESHEET, owner)
{
    owner->SetUseSpritesheet(true);
}

SpritesheetAddon::SpritesheetAddon(const rapidjson::Value& initialState, ParticleEmitter* owner)
    : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("Rows")) rows = initialState["Rows"].GetInt();
    if (initialState.HasMember("Columns")) columns = initialState["Columns"].GetInt();
    if (initialState.HasMember("AnimationSpeed")) animationSpeed = initialState["AnimationSpeed"].GetFloat();

    owner->SetUseSpritesheet(true);
}

SpritesheetAddon::~SpritesheetAddon()
{
    owner->SetUseSpritesheet(false);
}

void SpritesheetAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("Rows", rows, allocator);
    targetState.AddMember("Columns", columns, allocator);
    targetState.AddMember("AnimationSpeed", animationSpeed, allocator);
}

void SpritesheetAddon::Init(EmitterInstance* emitterInstance)
{
    currentFrame = 0.f;
    playTime = 0.f;
}

void SpritesheetAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    playTime     += deltaTime;
    currentFrame = playTime / timePerFrame;
}

void SpritesheetAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Base Addon");

    ImGui::PushItemWidth(100);

    ImGui::InputInt("Texture rows", &rows);
    ImGui::InputInt("Texture columns", &columns);
    if (ImGui::InputFloat("Animation speed", &animationSpeed))
    {
        if (animationSpeed <= 0) animationSpeed = 1.f;
    }

    timePerFrame = (rows * columns) / animationSpeed;

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
