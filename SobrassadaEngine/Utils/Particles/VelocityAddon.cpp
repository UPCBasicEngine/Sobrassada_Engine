#include "VelocityAddon.h"

#include "EmitterInstance.h"
#include "Interpolation.h"
#include "ParticleSystemComponent.h"

#include "imgui.h"
#include "imgui_curves.h"

VelocityAddon::VelocityAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::VELOCITY, owner)
{
}

VelocityAddon::VelocityAddon(const rapidjson::Value& initialState, ParticleEmitter* owner)
    : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("gravity")) gravity = initialState["gravity"].GetBool();
    if (initialState.HasMember("gravityValue")) gravityValue = initialState["gravityValue"].GetFloat();

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

    if (initialState.HasMember("useXCurve")) useXCurve = initialState["useXCurve"].GetBool();
    if (initialState.HasMember("useYCurve")) useYCurve = initialState["useYCurve"].GetBool();
    if (initialState.HasMember("useZCurve")) useZCurve = initialState["useZCurve"].GetBool();

    if (initialState.HasMember("xBezier"))
    {
        const rapidjson::Value& dataArray = initialState["xBezier"];
        bezierX[0]                        = dataArray[0].GetFloat();
        bezierX[1]                        = dataArray[1].GetFloat();
        bezierX[2]                        = dataArray[2].GetFloat();
        bezierX[3]                        = dataArray[3].GetFloat();
        bezierX[4]                        = dataArray[4].GetFloat();
    }

    if (initialState.HasMember("yBezier"))
    {
        const rapidjson::Value& dataArray = initialState["yBezier"];
        bezierY[0]                        = dataArray[0].GetFloat();
        bezierY[1]                        = dataArray[1].GetFloat();
        bezierY[2]                        = dataArray[2].GetFloat();
        bezierY[3]                        = dataArray[3].GetFloat();
        bezierY[4]                        = dataArray[4].GetFloat();
    }

    if (initialState.HasMember("zBezier"))
    {
        const rapidjson::Value& dataArray = initialState["zBezier"];
        bezierZ[0]                        = dataArray[0].GetFloat();
        bezierZ[1]                        = dataArray[1].GetFloat();
        bezierZ[2]                        = dataArray[2].GetFloat();
        bezierZ[3]                        = dataArray[3].GetFloat();
        bezierZ[4]                        = dataArray[4].GetFloat();
    }
}

VelocityAddon::~VelocityAddon()
{
}

void VelocityAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("gravity", gravity, allocator);
    targetState.AddMember("gravityValue", gravityValue, allocator);

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

    targetState.AddMember("useXCurve", useXCurve, allocator);
    targetState.AddMember("useYCurve", useYCurve, allocator);
    targetState.AddMember("useZCurve", useZCurve, allocator);

    rapidjson::Value xBezier(rapidjson::kArrayType);
    xBezier.PushBack(bezierX[0], allocator)
        .PushBack(bezierX[1], allocator)
        .PushBack(bezierX[2], allocator)
        .PushBack(bezierX[3], allocator)
        .PushBack(bezierX[4], allocator);
    targetState.AddMember("xBezier", xBezier, allocator);

    rapidjson::Value yBezier(rapidjson::kArrayType);
    yBezier.PushBack(bezierY[0], allocator)
        .PushBack(bezierY[1], allocator)
        .PushBack(bezierY[2], allocator)
        .PushBack(bezierY[3], allocator)
        .PushBack(bezierY[4], allocator);
    targetState.AddMember("yBezier", yBezier, allocator);

    rapidjson::Value zBezier(rapidjson::kArrayType);
    zBezier.PushBack(bezierZ[0], allocator)
        .PushBack(bezierZ[1], allocator)
        .PushBack(bezierZ[2], allocator)
        .PushBack(bezierZ[3], allocator)
        .PushBack(bezierZ[4], allocator);
    targetState.AddMember("zBezier", zBezier, allocator);
}

void VelocityAddon::Init(EmitterInstance* emitterInstance)
{
    for (auto& particle : emitterInstance->particles)
    {
        float finalX = 0;
        float finalY = 0;
        float finalZ = 0;

        if (!useXCurve) finalX = randomizeXSpeed ? rng->Float(xSpeed.x, xSpeed.y) : xSpeed.y;
        else finalX = xSpeed[0];

        if (!useYCurve) finalY = randomizeYSpeed ? rng->Float(ySpeed.x, ySpeed.y) : ySpeed.y;
        else finalY = ySpeed[0];

        if (!useZCurve) finalZ = randomizeZSpeed ? rng->Float(zSpeed.x, zSpeed.y) : zSpeed.y;
        else finalZ = zSpeed[0];

        particle.velocity = float3(finalX, finalY, finalZ);
    }
}

void VelocityAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (!IsEnabled()) return;

    for (int i = 0; i <= emitterInstance->particleVectorPos; ++i)
    {
        Particle& particle      = emitterInstance->particles[i];

        float valueOverLifetime = particle.currentLifetime / particle.lifeTime;

        if (useXCurve)
            particle.velocity.x =
                Interpolation::Lerp(xSpeed[0], xSpeed[1], ImGui::BezierValue(valueOverLifetime, bezierX));
        if (useYCurve)
            particle.velocity.y =
                Interpolation::Lerp(ySpeed[0], ySpeed[1], ImGui::BezierValue(valueOverLifetime, bezierY));
        if (useZCurve)
            particle.velocity.z =
                Interpolation::Lerp(zSpeed[0], zSpeed[1], ImGui::BezierValue(valueOverLifetime, bezierZ));

        particle.position = particle.position.Add(particle.direction.Mul((particle.velocity * deltaTime)));

        if (gravity) particle.position -= (float3::unitY * gravity * deltaTime);
    }
}

void VelocityAddon::RenderEditorInspector()
{
    if (!IsEnabled()) return;

    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Velocity Addon");

    // RENDER EDITOR STARTS

    ImGui::Spacing();

    ImGui::PushItemWidth(100);

    ImGui::Checkbox("Use gravity", &gravity);
    if (gravity)
    {
        ImGui::InputFloat("Gravity value", &gravityValue);
    }

    ImGui::PopItemWidth();

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("X Speed"))
    {
        if (ImGui::BeginCombo("X Velocity type", InterpolationAddonStrings[useXCurve ? 1 : 0]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) useXCurve = i;
            }
            ImGui::EndCombo();
        }

        if (useXCurve)
        {
            ImGui::Bezier("xBezier", bezierX);
            ImGui::InputFloat2("X Range", &xSpeed[0]);
        }
        else
        {
            ImGui::PushItemWidth(100);

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

            ImGui::PopItemWidth();
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Y Speed"))
    {
        if (ImGui::BeginCombo("Y Velocity type", InterpolationAddonStrings[useYCurve ? 1 : 0]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) useYCurve = i;
            }
            ImGui::EndCombo();
        }

        if (useYCurve)
        {
            ImGui::Bezier("yBezier", bezierY);
            ImGui::InputFloat2("Y Range", &ySpeed[0]);
        }
        else
        {
            ImGui::PushItemWidth(100);

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

            ImGui::PopItemWidth();
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Z Speed"))
    {
        if (ImGui::BeginCombo("Z Velocity type", InterpolationAddonStrings[useZCurve ? 1 : 0]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) useZCurve = i;
            }
            ImGui::EndCombo();
        }

        if (useZCurve)
        {
            ImGui::Bezier("zBezier", bezierZ);
            ImGui::InputFloat2("Z Range", &zSpeed[0]);
        }
        else
        {
            ImGui::PushItemWidth(100);

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

            ImGui::PopItemWidth();
        }
    }

    // RENDER EDITOR ENDS

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
