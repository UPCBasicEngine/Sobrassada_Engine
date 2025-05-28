#include "VelocityAddon.h"

#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

#include "imgui.h"
#include "imgui_curves.h"

VelocityAddon::VelocityAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::VELOCITY, owner)
{
}

VelocityAddon::VelocityAddon(const rapidjson::Value& initialState, ParticleEmitter* owner)
    : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("XSpeed"))
    {
        const rapidjson::Value& dataArray = initialState["XSpeed"];
        xSpeed                            = {dataArray[0].GetFloat(), dataArray[1].GetFloat()};
    }

    if (initialState.HasMember("YSpeed"))
    {
        const rapidjson::Value& dataArray = initialState["YSpeed"];
        ySpeed                            = {dataArray[0].GetFloat(), dataArray[1].GetFloat()};
    }

    if (initialState.HasMember("ZSpeed"))
    {
        const rapidjson::Value& dataArray = initialState["ZSpeed"];
        zSpeed                            = {dataArray[0].GetFloat(), dataArray[1].GetFloat()};
    }

    if (initialState.HasMember("RandX")) randomizeXSpeed = initialState["RandX"].GetBool();
    if (initialState.HasMember("RandY")) randomizeYSpeed = initialState["RandY"].GetBool();
    if (initialState.HasMember("RandZ")) randomizeZSpeed = initialState["RandZ"].GetBool();
}

VelocityAddon::~VelocityAddon()
{
}

void VelocityAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    rapidjson::Value xSpeedSave(rapidjson::kArrayType);
    xSpeedSave.PushBack(xSpeed.x, allocator).PushBack(xSpeed.y, allocator);
    targetState.AddMember("XSpeed", xSpeedSave, allocator);

    rapidjson::Value ySpeedSave(rapidjson::kArrayType);
    ySpeedSave.PushBack(ySpeed.x, allocator).PushBack(ySpeed.y, allocator);
    targetState.AddMember("YSpeed", ySpeedSave, allocator);

    rapidjson::Value zSpeedSave(rapidjson::kArrayType);
    zSpeedSave.PushBack(zSpeed.x, allocator).PushBack(zSpeed.y, allocator);
    targetState.AddMember("ZSpeed", zSpeedSave, allocator);

    targetState.AddMember("RandX", randomizeXSpeed, allocator);
    targetState.AddMember("RandY", randomizeYSpeed, allocator);
    targetState.AddMember("RandZ", randomizeZSpeed, allocator);
}

void VelocityAddon::Init(EmitterInstance* emitterInstance)
{
    for (auto& particle : emitterInstance->particles)
    {
        float finalX      = randomizeXSpeed ? rng->Float(xSpeed.x, xSpeed.y) : xSpeed.y;
        float finalY      = randomizeYSpeed ? rng->Float(ySpeed.x, ySpeed.y) : ySpeed.y;
        float finalZ      = randomizeZSpeed ? rng->Float(zSpeed.x, zSpeed.y) : zSpeed.y;

        particle.velocity = float3(finalX, finalY, finalZ);
    }
}

void VelocityAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (!IsEnabled()) return;

    for (auto& particle : emitterInstance->particles)
    {
        particle.position = particle.position.Add(particle.velocity * deltaTime);
    }
}

void VelocityAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;

    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Velocity Addon");

    ImGui::PushItemWidth(100);

    // RENDER EDITOR STARTS
    if (randomizeXSpeed)
    {
        ImGui::InputFloat("##MinLXSpeed", &xSpeed[0]);
        ImGui::SameLine();
    }
    ImGui::InputFloat("##MaxXSpeed", &xSpeed[1]);
    ImGui::SameLine();
    ImGui::Text("X Speed");
    ImGui::SameLine();
    ImGui::Checkbox("Rand.X Speed", &randomizeXSpeed);

    if (randomizeYSpeed)
    {
        ImGui::InputFloat("##MinLYSpeed", &ySpeed[0]);
        ImGui::SameLine();
    }
    ImGui::InputFloat("##MaxYSpeed", &ySpeed[1]);
    ImGui::SameLine();
    ImGui::Text("Y Speed");
    ImGui::SameLine();
    ImGui::Checkbox("Rand.Y Speed", &randomizeYSpeed);

    if (randomizeZSpeed)
    {
        ImGui::InputFloat("##MinLZSpeed", &zSpeed[0]);
        ImGui::SameLine();
    }
    ImGui::InputFloat("##MaxZSpeed", &zSpeed[1]);
    ImGui::SameLine();
    ImGui::Text("Z Speed");
    ImGui::SameLine();
    ImGui::Checkbox("Rand.Z Speed", &randomizeZSpeed);


    // RENDER EDITOR ENDS

    ImGui::PopItemWidth();

    ImGui::Bezier("easeOutSine", bezierX); // draw
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
