#include "SpritesheetAddon.h"

#include "EmitterInstance.h"

#include "imgui.h"

SpritesheetAddon::SpritesheetAddon() : ParticleAddon(ParticleAddonType::SPRITESHEET)
{
}

SpritesheetAddon::SpritesheetAddon(const rapidjson::Value& initialState) : ParticleAddon(initialState)
{
    if (initialState.HasMember("Rows")) rows = initialState["Rows"].GetInt();
    if (initialState.HasMember("Columns")) columns = initialState["Columns"].GetInt();
    if (initialState.HasMember("AnimationSpeed")) animationSpeed = initialState["AnimationSpeed"].GetFloat();
}

SpritesheetAddon::~SpritesheetAddon()
{
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
}

void SpritesheetAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
}

void SpritesheetAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Base Addon");

    ImGui::PushItemWidth(100);

    ImGui::InputInt("Texture rows", &rows);
    ImGui::InputInt("Texture columns", &columns);
    ImGui::InputFloat("Animation speed", &animationSpeed);

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
