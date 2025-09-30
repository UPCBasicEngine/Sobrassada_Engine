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

    if (initialState.HasMember("MinXTiles")) randomXTiles[0] = initialState["MinXTiles"].GetInt();
    if (initialState.HasMember("MaxXTiles")) randomXTiles[1] = initialState["MaxXTiles"].GetInt();

    if (initialState.HasMember("MinYTiles")) randomYTiles[0] = initialState["MinYTiles"].GetInt();
    if (initialState.HasMember("MaxYTiles")) randomYTiles[1] = initialState["MaxYTiles"].GetInt();
    if (initialState.HasMember("RandomizeTiles")) randomizeOffset = initialState["RandomizeTiles"].GetBool();

    owner->SetUseSpritesheet(true);

    timePerFrame = animationSpeed != 0 ? (rows * columns) / animationSpeed : 0;
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

    targetState.AddMember("MinXTiles", randomXTiles[0], allocator);
    targetState.AddMember("MaxXTiles", randomXTiles[1], allocator);
    targetState.AddMember("MinYTiles", randomYTiles[0], allocator);
    targetState.AddMember("MaxYTiles", randomYTiles[1], allocator);

    targetState.AddMember("RandomizeTiles", randomizeOffset, allocator);
}

void SpritesheetAddon::Init(EmitterInstance* emitterInstance)
{
    currentFrame = 0.f;
    playTime     = 0.f;

    if (!randomizeOffset) return;

    for (auto& particle : emitterInstance->particles)
    {
        particle.tileOffset.first  = rng->Int(randomXTiles[0], randomXTiles[1]);
        particle.tileOffset.second = rng->Int(randomYTiles[0], randomYTiles[1]);
    }
}

void SpritesheetAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    playTime     += deltaTime;
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

    ImGui::BeginDisabled(!randomizeOffset);
    if (ImGui::InputInt2("Random X tiles", &randomXTiles[0]))
    {
        if (randomXTiles[0] <= 0) randomXTiles[0] = 1;
        if (randomXTiles[1] <= 0) randomXTiles[1] = 1;
        else if (randomXTiles[0] > randomXTiles[1]) randomXTiles[1] = randomXTiles[0] + 1;
    }
    if (ImGui::InputInt2("Random Y tiles", &randomYTiles[0]))
    {
        if (randomYTiles[0] <= 0) randomYTiles[0] = 1;
        if (randomYTiles[1] <= 0) randomYTiles[1] = 1;
        else if (randomYTiles[0] > randomYTiles[1]) randomYTiles[1] = randomYTiles[0] + 1;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Checkbox("##RandomizeOffsets", &randomizeOffset);

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
        timePerFrame   = other->timePerFrame;

        for (int i = 0; i < 2; ++i)
        {
            randomXTiles[i] = other->randomXTiles[i];
            randomYTiles[i] = other->randomYTiles[i];
        }

        randomizeOffset = other->randomizeOffset;
    }
}
