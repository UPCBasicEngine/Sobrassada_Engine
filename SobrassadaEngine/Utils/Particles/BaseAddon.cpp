#include "BaseAddon.h"

#include "EmitterInstance.h"
#include "GameObject.h"
#include "Interpolation.h"
#include "ParticleSystemComponent.h"

#include "imgui.h"
#include "imgui_curves.h"

BaseAddon::BaseAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::BASE, owner)
{
}

BaseAddon::BaseAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("Duration")) duration = initialState["Duration"].GetFloat();
    if (initialState.HasMember("Loop")) loop = initialState["Loop"].GetBool();
    if (initialState.HasMember("MaxParticles")) maxParticles = initialState["MaxParticles"].GetInt();

    if (initialState.HasMember("RandomLifetime")) randomLifetime = initialState["RandomLifetime"].GetBool();
    if (initialState.HasMember("MinLifetime")) minLifetime = initialState["MinLifetime"].GetFloat();
    if (initialState.HasMember("MaxLifetime")) maxLifetime = initialState["MaxLifetime"].GetFloat();

    if (initialState.HasMember("randomizeSizeX")) randomizeSizeX = initialState["randomizeSizeX"].GetBool();
    if (initialState.HasMember("randomizeSizeY")) randomizeSizeY = initialState["randomizeSizeY"].GetBool();

    if (initialState.HasMember("sizeValuesX"))
    {
        const rapidjson::Value& dataArray = initialState["sizeValuesX"];
        sizeValuesX.x                     = dataArray[0].GetFloat();
        sizeValuesX.y                     = dataArray[1].GetFloat();
    }

    if (initialState.HasMember("sizeValuesY"))
    {
        const rapidjson::Value& dataArray = initialState["sizeValuesY"];
        sizeValuesY.x                     = dataArray[0].GetFloat();
        sizeValuesY.y                     = dataArray[1].GetFloat();
    }

    if (initialState.HasMember("useSizeCurveX")) useSizeCurveX = initialState["useSizeCurveX"].GetBool();
    if (initialState.HasMember("useSizeCurveY")) useSizeCurveY = initialState["useSizeCurveY"].GetBool();

    if (initialState.HasMember("sizeBezierX"))
    {
        const rapidjson::Value& dataArray = initialState["sizeBezierX"];
        sizeBezierX[0]                    = dataArray[0].GetFloat();
        sizeBezierX[1]                    = dataArray[1].GetFloat();
        sizeBezierX[2]                    = dataArray[2].GetFloat();
        sizeBezierX[3]                    = dataArray[3].GetFloat();
        sizeBezierX[4]                    = dataArray[4].GetFloat();
    }

    if (initialState.HasMember("sizeBezierY"))
    {
        const rapidjson::Value& dataArray = initialState["sizeBezierY"];
        sizeBezierY[0]                    = dataArray[0].GetFloat();
        sizeBezierY[1]                    = dataArray[1].GetFloat();
        sizeBezierY[2]                    = dataArray[2].GetFloat();
        sizeBezierY[3]                    = dataArray[3].GetFloat();
        sizeBezierY[4]                    = dataArray[4].GetFloat();
    }
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

    targetState.AddMember("RandomLifetime", randomLifetime, allocator);
    targetState.AddMember("MinLifetime", minLifetime, allocator);
    targetState.AddMember("MaxLifetime", maxLifetime, allocator);

    targetState.AddMember("randomizeSizeX", randomizeSizeX, allocator);
    targetState.AddMember("randomizeSizeY", randomizeSizeY, allocator);

    rapidjson::Value xSizeSave(rapidjson::kArrayType);
    xSizeSave.PushBack(sizeValuesX[0], allocator).PushBack(sizeValuesX[1], allocator);
    targetState.AddMember("sizeValuesX", xSizeSave, allocator);

    rapidjson::Value ySizeSave(rapidjson::kArrayType);
    ySizeSave.PushBack(sizeValuesY[0], allocator).PushBack(sizeValuesY[1], allocator);
    targetState.AddMember("sizeValuesY", ySizeSave, allocator);

    targetState.AddMember("useSizeCurveX", useSizeCurveX, allocator);
    targetState.AddMember("useSizeCurveY", useSizeCurveY, allocator);

    rapidjson::Value xBezier(rapidjson::kArrayType);
    xBezier.PushBack(sizeBezierX[0], allocator)
        .PushBack(sizeBezierX[1], allocator)
        .PushBack(sizeBezierX[2], allocator)
        .PushBack(sizeBezierX[3], allocator)
        .PushBack(sizeBezierX[4], allocator);
    targetState.AddMember("sizeBezierX", xBezier, allocator);

    rapidjson::Value yBezier(rapidjson::kArrayType);
    yBezier.PushBack(sizeBezierY[0], allocator)
        .PushBack(sizeBezierY[1], allocator)
        .PushBack(sizeBezierY[2], allocator)
        .PushBack(sizeBezierY[3], allocator)
        .PushBack(sizeBezierY[4], allocator);
    targetState.AddMember("sizeBezierY", yBezier, allocator);
}

void BaseAddon::Init(EmitterInstance* emitterInstance)
{
    emitterInstance->particles.clear();
    emitterInstance->particles.reserve(maxParticles);

    const float3 startingPosition = emitterInstance->GetOwner()->GetGlobalTransform().TranslatePart();
    emitterInstance->particles.assign(maxParticles, Particle(startingPosition));

    for (auto& particle : emitterInstance->particles)
    {
        particle.lifeTime = randomLifetime ? rng->Float(minLifetime, maxLifetime) : maxLifetime;

        float finalSizeX  = 1;
        float finalSizeY  = 1;

        if (!useSizeCurveX) finalSizeX = randomizeSizeX ? rng->Float(sizeValuesX[0], sizeValuesX[1]) : sizeValuesX[1];
        else finalSizeX = sizeValuesX[0];

        if (!useSizeCurveY) finalSizeY = randomizeSizeY ? rng->Float(sizeValuesY[0], sizeValuesY[1]) : sizeValuesY[1];
        else finalSizeY = sizeValuesY[0];

        particle.size = float2(finalSizeX, finalSizeY);
    }

    emitterInstance->currentEmissionTime = 0.f;
    emitterInstance->isEmitting          = true;
}

void BaseAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (emitterInstance->isEmitting)
    {
        float3 emitterPosition = emitterInstance->GetOwner()->GetGlobalTransform().TranslatePart();

        for (auto& particle : emitterInstance->particles)
        {
            if (particle.alive)
            {
                particle.currentLifetime += deltaTime;
                if (particle.currentLifetime >= particle.lifeTime)
                {
                    particle.currentLifetime = particle.lifeTime;
                    particle.alive           = false;
                }

                float valueOverLifetime = particle.currentLifetime / particle.lifeTime;

                if (useSizeCurveX)
                    particle.size.x = Interpolation::Lerp(
                        sizeValuesX[0], sizeValuesX[1], ImGui::BezierValue(valueOverLifetime, sizeBezierX)
                    );

                if (useSizeCurveY)
                    particle.size.y = Interpolation::Lerp(
                        sizeValuesY[0], sizeValuesY[1], ImGui::BezierValue(valueOverLifetime, sizeBezierY)
                    );
            }
            else
            {
                particle.alive           = true;
                particle.lifeTime        = randomLifetime ? rng->Float(minLifetime, maxLifetime) : maxLifetime;
                particle.currentLifetime = 0;
                particle.position        = float3(emitterPosition.x, emitterPosition.y, emitterPosition.z);

                float finalSizeX         = 1;
                float finalSizeY         = 1;

                if (!useSizeCurveX)
                    finalSizeX = randomizeSizeX ? rng->Float(sizeValuesX[0], sizeValuesX[1]) : sizeValuesX[1];
                else finalSizeX = sizeValuesX[0];

                if (!useSizeCurveY)
                    finalSizeY = randomizeSizeY ? rng->Float(sizeValuesY[0], sizeValuesY[1]) : sizeValuesY[1];
                else finalSizeY = sizeValuesY[0];

                particle.size = float2(finalSizeX, finalSizeY);
            }
        }

        emitterInstance->currentEmissionTime += deltaTime;
    }

    if (emitterInstance->currentEmissionTime > duration)
    {
        emitterInstance->isEmitting = false;
    }
}

void BaseAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Base Addon");

    ImGui::PushItemWidth(100);

    ImGui::Checkbox("Loop", &loop);
    ImGui::InputFloat("Duration", &duration, 0.05f, 1.f);
    ImGui::InputInt("Max Particles", &maxParticles, 5, 10);

    if (randomLifetime)
    {
        ImGui::InputFloat("##MinLifetime", &minLifetime);
        ImGui::SameLine();
    }
    ImGui::InputFloat("##MaxLifetime", &maxLifetime);
    ImGui::SameLine();
    ImGui::Text("Lifetime");
    ImGui::SameLine();
    ImGui::Checkbox("Rand##Lifetime", &randomLifetime);

    ImGui::PopItemWidth();

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("X Size"))
    {
        if (ImGui::BeginCombo("Particle size##X", InterpolationAddonStrings[useSizeCurveX ? 1 : 0]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) useSizeCurveX = i;
            }
            ImGui::EndCombo();
        }

        if (useSizeCurveX)
        {
            ImGui::Bezier("SizeBezier##X", sizeBezierX);
            ImGui::InputFloat2("X Particle Range", &sizeValuesX[0]);
        }
        else
        {
            ImGui::PushItemWidth(100);
            if (randomizeSizeX)
            {
                ImGui::InputFloat("##MinSizeX", &sizeValuesX[0]);
                ImGui::SameLine();
            }
            ImGui::InputFloat("##MaxSizeX", &sizeValuesX[1]);
            ImGui::SameLine();
            ImGui::Text("X Size");
            ImGui::SameLine();
            ImGui::Checkbox("Rand##X", &randomizeSizeX);

            ImGui::PopItemWidth();
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Y Size"))
    {
        if (ImGui::BeginCombo("Particle size##Y", InterpolationAddonStrings[useSizeCurveY ? 1 : 0]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) useSizeCurveY = i;
            }
            ImGui::EndCombo();
        }

        if (useSizeCurveY)
        {
            ImGui::Bezier("SizeBezier#Y", sizeBezierY);
            ImGui::InputFloat2("Y Particle Range", &sizeValuesY[0]);
        }
        else
        {
            ImGui::PushItemWidth(100);
            if (randomizeSizeY)
            {
                ImGui::InputFloat("##MinSizeY", &sizeValuesY[0]);
                ImGui::SameLine();
            }
            ImGui::InputFloat("##MaxSizeY", &sizeValuesY[1]);
            ImGui::SameLine();
            ImGui::Text("Y Size");
            ImGui::SameLine();
            ImGui::Checkbox("Rand##Y", &randomizeSizeY);

            ImGui::PopItemWidth();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
