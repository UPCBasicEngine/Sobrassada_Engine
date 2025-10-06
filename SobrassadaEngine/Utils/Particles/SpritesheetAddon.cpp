#include "SpritesheetAddon.h"

#include "EmitterInstance.h"
#include "Globals.h"
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
    if (initialState.HasMember("RandomizeTiles")) randomizeOffset = initialState["RandomizeTiles"].GetBool();

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
    targetState.AddMember("RandomizeTiles", randomizeOffset, allocator);
}

void SpritesheetAddon::Init(EmitterInstance* emitterInstance)
{
    currentFrame = 0.f;

    if (!randomizeOffset) return;

    for (auto& particle : emitterInstance->particles)
    {
        particle.tileOffset = rng->Int(0, (rows * columns) - 1);
    }
}

void SpritesheetAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    currentFrame += deltaTime * animationSpeed;
}

void SpritesheetAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Spritesheet Addon");

    ImGui::PushItemWidth(100);

    if (ImGui::InputInt("Texture rows", &rows))
    {
        if (rows <= 0) rows = 1;
    }
    if (ImGui::InputInt("Texture columns", &columns))
    {
        if (columns <= 0) columns = 1;
    }
    if (ImGui::InputFloat("Animation speed", &animationSpeed))
    {
        // if (animationSpeed <= 0) animationSpeed = 1.f;
    }


    ImGui::SameLine();
    ImGui::Checkbox("Randomize Offsets", &randomizeOffset);

    timePerFrame = animationSpeed != 0 ? (rows * columns) / animationSpeed : 0;

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void SpritesheetAddon::Duplicate(ParticleAddon* reference)
{
    SpritesheetAddon* other = reinterpret_cast<SpritesheetAddon*>(reference);

    if (other)
    {
        rows           = other->rows;
        columns        = other->columns;

        animationSpeed = other->animationSpeed;

        randomizeOffset = other->randomizeOffset;
    }
}
