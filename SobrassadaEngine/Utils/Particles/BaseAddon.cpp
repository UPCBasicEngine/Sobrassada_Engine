#include "BaseAddon.h"

#include "imgui.h"

BaseAddon::BaseAddon(ParticleEmitter* owner) : ParticleAddon(owner, ParticleAddonType::BASE)
{
}

BaseAddon::BaseAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("Duration")) duration = initialState["Duration"].GetFloat();
    if (initialState.HasMember("Loop")) loop = initialState["Loop"].GetBool();
    if (initialState.HasMember("MaxParticles")) maxParticles = initialState["MaxParticles"].GetInt();
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
}

void BaseAddon::Init() const
{
    // INITIALIZE EMITTER PARTICLES VECTOR WITH MAX_PARITCLES AND
}

void BaseAddon::Update(float deltaTime) const
{
    if (!IsEnabled()) return;

    // ADD PARTICLES IF EMITTER PARTICLES < MAX_PARTICLES
}

void BaseAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;

    ImGui::Text("BASE ADDON");

    ImGui::Checkbox("Loop", &loop);
    ImGui::DragFloat("Duration", &duration, 0.05f, 0.f, 20.f);
    ImGui::DragInt("Max Particles", &maxParticles, 1, 0, 500);

    ImGui::Separator();


}
