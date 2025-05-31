#include "AreaAddon.h"

#include "EmitterInstance.h"
#include "ParticleEmitter.h"

#include "imgui.h"

AreaAddon::AreaAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::AREA, owner)
{
}

AreaAddon::AreaAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
}

AreaAddon::~AreaAddon()
{
}

void AreaAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
}

void AreaAddon::Init(EmitterInstance* emitterInstance)
{
}

void AreaAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
}

void AreaAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Area Addon");
}
