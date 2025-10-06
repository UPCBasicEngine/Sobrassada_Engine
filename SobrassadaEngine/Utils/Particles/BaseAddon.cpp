#include "BaseAddon.h"

#include "AreaAddon.h"
#include "EmitterInstance.h"
#include "GameObject.h"
#include "Interpolation.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

// #include "imgui.h"
#include "imgui_curve_editor.h"
#include "imgui_curves.h"

BaseAddon::BaseAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::BASE, owner)
{
    ResetCurveEditorPoints(curveEditorPoints);
    ResetCurveEditorPoints(curveEditorXPoints);
    ResetCurveEditorPoints(curveEditorYPoints);
}

BaseAddon::BaseAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
    ResetCurveEditorPoints(curveEditorPoints);
    ResetCurveEditorPoints(curveEditorXPoints);
    ResetCurveEditorPoints(curveEditorYPoints);

    if (initialState.HasMember("Duration")) duration = initialState["Duration"].GetFloat();
    if (initialState.HasMember("Loop")) loop = initialState["Loop"].GetBool();
    if (initialState.HasMember("particlesPerSecond")) particlesPerSecond = initialState["particlesPerSecond"].GetInt();
    if (initialState.HasMember("MaxParticles")) maxParticles = initialState["MaxParticles"].GetInt();

    if (initialState.HasMember("RandomLifetime")) randomLifetime = initialState["RandomLifetime"].GetBool();
    if (initialState.HasMember("MinLifetime")) minLifetime = initialState["MinLifetime"].GetFloat();
    if (initialState.HasMember("MaxLifetime")) maxLifetime = initialState["MaxLifetime"].GetFloat();

    if (initialState.HasMember("randomizeSizeCombined"))
        randomizeSizeCombined = initialState["randomizeSizeCombined"].GetBool();
    if (initialState.HasMember("randomizeSizeX")) randomizeSizeX = initialState["randomizeSizeX"].GetBool();
    if (initialState.HasMember("randomizeSizeY")) randomizeSizeY = initialState["randomizeSizeY"].GetBool();

    if (initialState.HasMember("combinedSize"))
    {
        const rapidjson::Value& dataArray = initialState["combinedSize"];
        combinedSize.x                    = dataArray[0].GetFloat();
        combinedSize.y                    = dataArray[1].GetFloat();
    }

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

    if (initialState.HasMember("sizeBezierCombined"))
    {
        const rapidjson::Value& dataArray = initialState["sizeBezierCombined"];
        sizeBezierCombined[0]             = dataArray[0].GetFloat();
        sizeBezierCombined[1]             = dataArray[1].GetFloat();
        sizeBezierCombined[2]             = dataArray[2].GetFloat();
        sizeBezierCombined[3]             = dataArray[3].GetFloat();
        sizeBezierCombined[4]             = dataArray[4].GetFloat();
    }

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

    if (initialState.HasMember("rotation"))
    {
        const rapidjson::Value& dataArray = initialState["sizeBezierY"];
        rotation[0]                       = dataArray[0].GetFloat();
        rotation[1]                       = dataArray[1].GetFloat();
    }

    if (initialState.HasMember("randomRotation")) randomRotation = initialState["randomRotation"].GetBool();
    if (initialState.HasMember("burst")) burst = initialState["burst"].GetBool();
    if (initialState.HasMember("respawnLoop")) respawnLoop = initialState["respawnLoop"].GetBool();

    if (initialState.HasMember("MaxCurveEditorPoints"))
    {
        int minMaxPoints = initialState["MaxCurveEditorPoints"].GetInt();

        minMaxPoints     = minMaxPoints < MaxCurveEditorPoints ? minMaxPoints : MaxCurveEditorPoints;

        if (initialState.HasMember("curveEditorPoints"))
        {
            const rapidjson::Value& dataArray = initialState["curveEditorPoints"];

            int currentPoint                  = 0;
            for (int i = 1; i < (minMaxPoints * 2); i += 2, currentPoint++)
            {
                curveEditorPoints[currentPoint].x = dataArray[i - 1].GetFloat();
                curveEditorPoints[currentPoint].y = dataArray[i].GetFloat();
            }
        }

        if (initialState.HasMember("curveEditorXPoints"))
        {
            const rapidjson::Value& dataArray = initialState["curveEditorXPoints"];

            int currentPoint                  = 0;
            for (int i = 1; i < (minMaxPoints * 2); i += 2, currentPoint++)
            {
                curveEditorXPoints[currentPoint].x = dataArray[i - 1].GetFloat();
                curveEditorXPoints[currentPoint].y = dataArray[i].GetFloat();
            }
        }

        if (initialState.HasMember("curveEditorYPoints"))
        {
            const rapidjson::Value& dataArray = initialState["curveEditorYPoints"];

            int currentPoint                  = 0;
            for (int i = 1; i < (minMaxPoints * 2); i += 2, currentPoint++)
            {
                curveEditorYPoints[currentPoint].x = dataArray[i - 1].GetFloat();
                curveEditorYPoints[currentPoint].y = dataArray[i].GetFloat();
            }
        }
    }

    if (initialState.HasMember("updateXYApart")) updateXYApart = initialState["updateXYApart"].GetBool();
    if (initialState.HasMember("sizeInterpolation"))
        sizeInterpolation = ParticleInterpolationType(initialState["sizeInterpolation"].GetInt());
    if (initialState.HasMember("sizeInterpolationX"))
        sizeInterpolationX = ParticleInterpolationType(initialState["sizeInterpolationX"].GetInt());
    if (initialState.HasMember("sizeInterpolationY"))
        sizeInterpolationY = ParticleInterpolationType(initialState["sizeInterpolationY"].GetInt());
}

BaseAddon::~BaseAddon()
{
}

void BaseAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("Duration", duration, allocator);
    targetState.AddMember("Loop", loop, allocator);
    targetState.AddMember("particlesPerSecond", particlesPerSecond, allocator);
    targetState.AddMember("MaxParticles", maxParticles, allocator);

    targetState.AddMember("RandomLifetime", randomLifetime, allocator);
    targetState.AddMember("MinLifetime", minLifetime, allocator);
    targetState.AddMember("MaxLifetime", maxLifetime, allocator);

    targetState.AddMember("randomizeSizeCombined", randomizeSizeCombined, allocator);
    targetState.AddMember("randomizeSizeX", randomizeSizeX, allocator);
    targetState.AddMember("randomizeSizeY", randomizeSizeY, allocator);

    rapidjson::Value combinedSizeSave(rapidjson::kArrayType);
    combinedSizeSave.PushBack(combinedSize[0], allocator).PushBack(combinedSize[1], allocator);
    targetState.AddMember("combinedSize", combinedSizeSave, allocator);

    rapidjson::Value xSizeSave(rapidjson::kArrayType);
    xSizeSave.PushBack(sizeValuesX[0], allocator).PushBack(sizeValuesX[1], allocator);
    targetState.AddMember("sizeValuesX", xSizeSave, allocator);

    rapidjson::Value ySizeSave(rapidjson::kArrayType);
    ySizeSave.PushBack(sizeValuesY[0], allocator).PushBack(sizeValuesY[1], allocator);
    targetState.AddMember("sizeValuesY", ySizeSave, allocator);

    rapidjson::Value combinedBezier(rapidjson::kArrayType);
    combinedBezier.PushBack(sizeBezierCombined[0], allocator)
        .PushBack(sizeBezierCombined[1], allocator)
        .PushBack(sizeBezierCombined[2], allocator)
        .PushBack(sizeBezierCombined[3], allocator)
        .PushBack(sizeBezierCombined[4], allocator);
    targetState.AddMember("sizeBezierCombined", combinedBezier, allocator);

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

    rapidjson::Value rotationSave(rapidjson::kArrayType);
    rotationSave.PushBack(rotation[0], allocator).PushBack(rotation[1], allocator);
    targetState.AddMember("rotation", rotationSave, allocator);

    targetState.AddMember("randomRotation", randomRotation, allocator);
    targetState.AddMember("burst", burst, allocator);
    targetState.AddMember("respawnLoop", respawnLoop, allocator);

    targetState.AddMember("MaxCurveEditorPoints", MaxCurveEditorPoints, allocator);

    rapidjson::Value combinedEditorCurveSave(rapidjson::kArrayType);
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        combinedEditorCurveSave.PushBack(curveEditorPoints[i].x, allocator).PushBack(curveEditorPoints[i].y, allocator);
    }
    targetState.AddMember("curveEditorPoints", combinedEditorCurveSave, allocator);

    rapidjson::Value editorCurveXSave(rapidjson::kArrayType);
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        editorCurveXSave.PushBack(curveEditorXPoints[i].x, allocator).PushBack(curveEditorXPoints[i].y, allocator);
    }
    targetState.AddMember("curveEditorXPoints", editorCurveXSave, allocator);

    rapidjson::Value editorCurveYSave(rapidjson::kArrayType);
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        editorCurveYSave.PushBack(curveEditorYPoints[i].x, allocator).PushBack(curveEditorYPoints[i].y, allocator);
    }
    targetState.AddMember("curveEditorYPoints", editorCurveYSave, allocator);

    targetState.AddMember("updateXYApart", updateXYApart, allocator);
    targetState.AddMember("sizeInterpolation", (int)sizeInterpolation, allocator);
    targetState.AddMember("sizeInterpolationX", (int)sizeInterpolationX, allocator);
    targetState.AddMember("sizeInterpolationY", (int)sizeInterpolationY, allocator);
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

        InitializeParticleSize(particle);

        particle.rotation  = randomRotation ? rng->Float(rotation[0], rotation[1]) : rotation[1];
        particle.rotation *= DEGREE_RAD_CONV;
    }

    emitterInstance->currentEmissionTime = 0.f;

    emitterInstance->particleVectorPos   = 0;
    if (burst) emitterInstance->particleVectorPos = maxParticles - 1;

    spawnDeltaTime              = (1.f / particlesPerSecond) + 0.01f;
    emitterInstance->isEmitting = true;
}

void BaseAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (emitterInstance->isEmitting)
    {
        emitterInstance->currentEmissionTime += deltaTime;
        spawnDeltaTime                       += deltaTime;

        AreaAddon* areaAddon                  = owner->GetAddon<AreaAddon*>();
        if (areaAddon) areaAddon->UpdateShapesTransforms(emitterInstance->GetOwner()->GetGlobalTransform());

        float3 emitterPosition = emitterInstance->GetOwner()->GetGlobalTransform().TranslatePart();

        while (spawnDeltaTime > (1.f / particlesPerSecond))
        {
            spawnDeltaTime -= (1.f / particlesPerSecond);
            emitterInstance->particleVectorPos++;
        }

        if (emitterInstance->particleVectorPos >= maxParticles) emitterInstance->particleVectorPos = maxParticles - 1;

        for (int i = 0; i <= emitterInstance->particleVectorPos; ++i)
        {
            Particle& particle = emitterInstance->particles[i];

            if (particle.alive)
            {
                particle.currentLifetime += deltaTime;
                if (particle.currentLifetime >= particle.lifeTime)
                {
                    particle.currentLifetime = particle.lifeTime;
                    particle.alive           = false;
                }

                float valueOverLifetime = particle.currentLifetime / particle.lifeTime;

                UpdateParticleSize(particle, valueOverLifetime);
            }
            else
            {
                particle.alive           = true;
                particle.lifeTime        = randomLifetime ? rng->Float(minLifetime, maxLifetime) : maxLifetime;
                particle.currentLifetime = 0;

                if (areaAddon) areaAddon->AssignPositionDirection(particle);
                else
                {
                    particle.position  = float3(emitterPosition.x, emitterPosition.y, emitterPosition.z);
                    particle.direction = float3::one.Normalized();
                }

                InitializeParticleSize(particle);

                particle.rotation  = randomRotation ? rng->Float(rotation[0], rotation[1]) : rotation[1];
                particle.rotation *= DEGREE_RAD_CONV;
            }
        }
    }

    if (emitterInstance->currentEmissionTime > duration && !loop)
    {
        emitterInstance->isEmitting        = false;
        emitterInstance->particleVectorPos = 0;
        spawnDeltaTime                     = 0;
    }
    else if (emitterInstance->currentEmissionTime > duration && loop && respawnLoop)
    {
        emitterInstance->Spawn();
    }
}

void BaseAddon::RenderEditorInspector()
{
    bool anyChange = false;

    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Base Addon");

    ImGui::PushItemWidth(100);

    ImGui::Checkbox("Loop", &loop);
    ImGui::SameLine();
    ImGui::BeginDisabled(!loop);
    ImGui::Checkbox("Respawn", &respawnLoop);
    ImGui::EndDisabled();

    if (ImGui::InputFloat("Duration", &duration, 0.05f, 1.f)) anyChange = true;
    if (ImGui::InputInt("Emitting rate", &particlesPerSecond, 5, 10))
    {
        particlesPerSecond = particlesPerSecond < 1 ? 1 : particlesPerSecond;
    }
    if (ImGui::InputInt("Max Particles", &maxParticles, 5, 10))
    {
        owner->Stop();
        maxParticles = maxParticles < 1 ? 1 : maxParticles;
    }
    ImGui::Checkbox("Burst", &burst);

    if (randomLifetime)
    {
        if (ImGui::InputFloat("##MinLifetime", &minLifetime)) anyChange = true;
        ImGui::SameLine();
    }
    if (ImGui::InputFloat("##MaxLifetime", &maxLifetime)) anyChange = true;
    ImGui::SameLine();
    (ImGui::Text("Lifetime"));
    ImGui::SameLine();
    ImGui::Checkbox("Rand##Lifetime", &randomLifetime);

    if (randomRotation)
    {
        ImGui::InputFloat("##MinRotation", &rotation[0]);
        ImGui::SameLine();
    }
    ImGui::InputFloat("##MaxRotation", &rotation[1]);
    ImGui::SameLine();
    ImGui::Text("Rotation");
    ImGui::SameLine();
    ImGui::Checkbox("Rand##Rotation", &randomRotation);

    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Checkbox("Update X,Y apart", &updateXYApart);

    if (!updateXYApart)
    {
        if (ImGui::BeginCombo("Particle size##Combined", InterpolationAddonStrings[(int)sizeInterpolation]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i]))
                {
                    sizeInterpolation = ParticleInterpolationType(i);
                    anyChange         = true;
                }
            }
            ImGui::EndCombo();
        }

        switch (sizeInterpolation)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        {
            ImGui::PushItemWidth(100);
            if (randomizeSizeCombined)
            {
                if (ImGui::InputFloat("##MinSizeCombined", &combinedSize[0])) anyChange = true;
                ImGui::SameLine();
            }
            if (ImGui::InputFloat("##MaxSizeCombined", &combinedSize[1])) anyChange = true;
            ImGui::SameLine();
            ImGui::Text("Combined Size");
            ImGui::SameLine();
            ImGui::Checkbox("Rand##Combined", &randomizeSizeCombined);

            ImGui::PopItemWidth();

            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            if (ImGui::Bezier("SizeBezier##Combined", sizeBezierCombined)) anyChange = true;
            if (ImGui::InputFloat2("Particle Range", &combinedSize[0])) anyChange = true;

            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            ImGui::Spacing();

            if (ImGui::Curve(
                    "Combined Size Curve", ImVec2(400, 100), MaxCurveEditorPoints, curveEditorPoints, &curveEditorIndex,
                    ImVec2(0.f, curveEditorValueRange.x), ImVec2(1.f, curveEditorValueRange.y)
                ))
                anyChange = true;

            if (ImGui::InputFloat2("Combined Size", &curveEditorValueRange[0]))
            {
                if (combinedSize.x < 0) combinedSize.x = 0;
                if (combinedSize.y < 0) combinedSize.y = 0;

                anyChange = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Points")) ResetCurveEditorPoints(curveEditorPoints);

            break;
        }
        default:
            break;
        }
    }
    else
    {
        if (ImGui::CollapsingHeader("X Size"))
        {
            if (ImGui::BeginCombo("Particle size##X", InterpolationAddonStrings[(int)sizeInterpolationX]))
            {
                for (int i = 0; i < InterpolationAddonStringsSize; ++i)
                {
                    if (ImGui::Selectable(InterpolationAddonStrings[i]))
                    {
                        sizeInterpolationX = ParticleInterpolationType(i);
                        anyChange          = true;
                    }
                }
                ImGui::EndCombo();
            }

            switch (sizeInterpolationX)
            {
            case ParticleInterpolationType::FIXED_VALUES:
            {
                ImGui::PushItemWidth(100);
                if (randomizeSizeX)
                {
                    if (ImGui::InputFloat("##MinSizeX", &sizeValuesX[0])) anyChange = true;
                    ImGui::SameLine();
                }
                if (ImGui::InputFloat("##MaxSizeX", &sizeValuesX[1])) anyChange = true;
                ImGui::SameLine();
                ImGui::Text("X Size");
                ImGui::SameLine();
                ImGui::Checkbox("Rand##X", &randomizeSizeX);

                ImGui::PopItemWidth();
                break;
            }
            case ParticleInterpolationType::BEZIER_SINGLE:
            {
                if (ImGui::Bezier("SizeBezier##X", sizeBezierX)) anyChange = true;
                if (ImGui::InputFloat2("X Particle Range", &sizeValuesX[0])) anyChange = true;
                break;
            }
            case ParticleInterpolationType::CURVE_EDITOR:
            {
                if (ImGui::Curve(
                        "X Size Curve", ImVec2(400, 100), MaxCurveEditorPoints, curveEditorXPoints, &curveEditorIndexX,
                        ImVec2(0.f, curveEditorValueRange.x), ImVec2(1.f, curveEditorValueRange.y)
                    ))
                    anyChange = true;

                if (ImGui::InputFloat2("X Size Range", &sizeValuesX[0]))
                {
                    if (sizeValuesX.x < 0) sizeValuesX.x = 0;
                    if (sizeValuesX.y < 0) sizeValuesX.y = 0;

                    anyChange = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Points##X")) ResetCurveEditorPoints(curveEditorXPoints);

                break;
            }
            default:
                break;
            }
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Y Size"))
        {
            if (ImGui::BeginCombo("Particle size##Y", InterpolationAddonStrings[(int)sizeInterpolationY]))
            {
                for (int i = 0; i < InterpolationAddonStringsSize; ++i)
                {
                    if (ImGui::Selectable(InterpolationAddonStrings[i]))
                    {
                        sizeInterpolationY = ParticleInterpolationType(i);
                        anyChange          = true;
                    }
                }
                ImGui::EndCombo();
            }

            switch (sizeInterpolationY)
            {
            case ParticleInterpolationType::FIXED_VALUES:
            {
                ImGui::PushItemWidth(100);
                if (randomizeSizeY)
                {
                    if (ImGui::InputFloat("##MinSizeY", &sizeValuesY[0])) anyChange = true;
                    ImGui::SameLine();
                }
                if (ImGui::InputFloat("##MaxSizeY", &sizeValuesY[1])) anyChange = true;
                ImGui::SameLine();
                ImGui::Text("Y Size");
                ImGui::SameLine();
                ImGui::Checkbox("Rand##Y", &randomizeSizeY);

                ImGui::PopItemWidth();
                break;
            }
            case ParticleInterpolationType::BEZIER_SINGLE:
            {
                if (ImGui::Bezier("SizeBezier#Y", sizeBezierY)) anyChange = true;
                if (ImGui::InputFloat2("Y Particle Range", &sizeValuesY[0])) anyChange = true;
                break;
            }
            case ParticleInterpolationType::CURVE_EDITOR:
            {
                if (ImGui::Curve(
                        "Y Size Curve", ImVec2(400, 100), MaxCurveEditorPoints, curveEditorYPoints, &curveEditorIndexX,
                        ImVec2(0.f, curveEditorValueRange.x), ImVec2(1.f, curveEditorValueRange.y)
                    ))
                    anyChange = true;

                if (ImGui::InputFloat2("Y Size Range", &sizeValuesY[0]))
                {
                    if (sizeValuesY.x < 0) sizeValuesY.x = 0;
                    if (sizeValuesY.y < 0) sizeValuesY.y = 0;
                    anyChange = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Points##Y")) ResetCurveEditorPoints(curveEditorYPoints);

                break;
            }
            default:
                break;
            }
        }
    }

    if (anyChange) owner->UpdateAABB();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void BaseAddon::Duplicate(ParticleAddon* reference)
{
    BaseAddon* other = reinterpret_cast<BaseAddon*>(reference);

    if (other)
    {
        duration              = other->duration;
        loop                  = other->loop;
        respawnLoop           = other->respawnLoop;
        maxParticles          = other->maxParticles;

        randomLifetime        = other->randomLifetime;
        minLifetime           = other->minLifetime;
        maxLifetime           = other->maxLifetime;

        randomRotation        = other->randomRotation;
        rotation              = other->rotation;

        updateXYApart         = other->updateXYApart;

        sizeInterpolation     = other->sizeInterpolation;
        sizeInterpolationX    = other->sizeInterpolationX;
        sizeInterpolationY    = other->sizeInterpolationY;

        randomizeSizeCombined = other->randomizeSizeCombined;
        randomizeSizeX        = other->randomizeSizeX;
        randomizeSizeY        = other->randomizeSizeY;
        combinedSize          = other->combinedSize;
        sizeValuesX           = other->sizeValuesX;
        sizeValuesY           = other->sizeValuesY;

        for (int i = 0; i < 5; ++i)
        {
            sizeBezierCombined[i] = other->sizeBezierCombined[i];
            sizeBezierX[i]        = other->sizeBezierX[i];
            sizeBezierY[i]        = other->sizeBezierY[i];
        }

        for (int i = 0; i < MaxCurveEditorPoints; ++i)
        {
            curveEditorPoints[i]  = other->curveEditorPoints[i];
            curveEditorXPoints[i] = other->curveEditorXPoints[i];
            curveEditorYPoints[i] = other->curveEditorYPoints[i];
        }

        particlesPerSecond = other->particlesPerSecond;
        burst              = other->burst;
    }
}

void BaseAddon::AssignMaxValues(ParticleValues& particleValue)
{
    particleValue.lifeTime = !loop ? fmin(maxLifetime, duration) : maxLifetime;

    if (!updateXYApart)
    {
        switch (sizeInterpolation)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            if (particleValue.size.x < combinedSize[1]) particleValue.size.x = combinedSize[1];
            if (particleValue.size.y < combinedSize[1]) particleValue.size.y = combinedSize[1];
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            float value = ImGui::CurveValue(1.f, MaxCurveEditorPoints, curveEditorPoints);
            if (particleValue.size.x < value) particleValue.size.x = value;
            if (particleValue.size.y < value) particleValue.size.y = value;
            break;
        }
        default:
            break;
        }
    }
    else
    {
        // X Values
        switch (sizeInterpolationX)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            if (particleValue.size.x < sizeValuesX[1]) particleValue.size.x = sizeValuesX[1];
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            float value = ImGui::CurveValue(1.f, MaxCurveEditorPoints, curveEditorXPoints);
            if (particleValue.size.x < value) particleValue.size.x = value;
            break;
        }
        default:
            break;
        }

        // Y Values
        switch (sizeInterpolationY)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            if (particleValue.size.y < sizeValuesY[1]) particleValue.size.y = sizeValuesY[1];
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            float value = ImGui::CurveValue(1.f, MaxCurveEditorPoints, curveEditorYPoints);
            if (particleValue.size.y < value) particleValue.size.y = value;
            break;
        }
        default:
            break;
        }
    }
}

void BaseAddon::ResetCurveEditorPoints(ImVec2* pointsToReset)
{
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        pointsToReset[i].x = (float)i / 10.f;
        pointsToReset[i].y = (float)i / 10.f;
    }

    pointsToReset[0].x = ImGui::CurveTerminator;
}

void BaseAddon::InitializeParticleSize(Particle& particle)
{
    float finalSizeX = 1;
    float finalSizeY = 1;

    if (!updateXYApart)
    {
        switch (sizeInterpolation)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        {
            float value = randomizeSizeCombined ? rng->Float(combinedSize[0], combinedSize[1]) : combinedSize[1];
            finalSizeX  = value;
            finalSizeY  = value;
            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            finalSizeX = combinedSize[0];
            finalSizeY = combinedSize[0];
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            float value = ImGui::CurveValue(0.f, MaxCurveEditorPoints, curveEditorPoints);
            finalSizeX  = value;
            finalSizeY  = value;
            break;
        }
        default:
            break;
        }
    }
    else
    {
        // X Values
        switch (sizeInterpolationX)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        {
            finalSizeX = randomizeSizeX ? rng->Float(sizeValuesX[0], sizeValuesX[1]) : sizeValuesX[1];
            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            finalSizeX = sizeValuesX[0];
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            finalSizeX = ImGui::CurveValue(0.f, MaxCurveEditorPoints, curveEditorXPoints);
            break;
        }
        default:
            break;
        }

        // Y Values
        switch (sizeInterpolationY)
        {
        case ParticleInterpolationType::FIXED_VALUES:
        {
            finalSizeY = randomizeSizeY ? rng->Float(sizeValuesY[0], sizeValuesY[1]) : sizeValuesY[1];
            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            finalSizeY = sizeValuesY[0];
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            finalSizeY = ImGui::CurveValue(0.f, MaxCurveEditorPoints, curveEditorYPoints);
            break;
        }
        default:
            break;
        }
    }

    particle.size = float2(finalSizeX, finalSizeY);
}

void BaseAddon::UpdateParticleSize(Particle& particle, float valueOverLifetime)
{
    if (!updateXYApart)
    {
        switch (sizeInterpolation)
        {
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            float value = Interpolation::Lerp(
                combinedSize[0], combinedSize[1], ImGui::BezierValue(valueOverLifetime, sizeBezierCombined)
            );
            particle.size.x = value;
            particle.size.y = value;
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            float value     = ImGui::CurveValue(valueOverLifetime, MaxCurveEditorPoints, curveEditorPoints);
            particle.size.x = value;
            particle.size.y = value;
            break;
        }
        default:
            break;
        }
    }
    else
    {
        // SIZE X
        switch (sizeInterpolationX)
        {
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            particle.size.x =
                Interpolation::Lerp(sizeValuesX[0], sizeValuesX[1], ImGui::BezierValue(valueOverLifetime, sizeBezierX));
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            particle.size.x = ImGui::CurveValue(valueOverLifetime, MaxCurveEditorPoints, curveEditorXPoints);
            break;
        }
        default:
            break;
        }

        // SIZE Y
        switch (sizeInterpolationY)
        {
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            particle.size.y =
                Interpolation::Lerp(sizeValuesY[0], sizeValuesY[1], ImGui::BezierValue(valueOverLifetime, sizeBezierY));
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            particle.size.y = ImGui::CurveValue(valueOverLifetime, MaxCurveEditorPoints, curveEditorYPoints);
            break;
        }
        default:
            break;
        }
    }
}
