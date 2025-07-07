#include "VelocityAddon.h"

#include "EmitterInstance.h"
#include "Interpolation.h"
#include "ParticleSystemComponent.h"

// #include "imgui.h"
#include "imgui_curve_editor.h"
#include "imgui_curves.h"

VelocityAddon::VelocityAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::VELOCITY, owner)
{
    ResetCurveEditorPoints(curveEditorPointsX);
    ResetCurveEditorPoints(curveEditorPointsY);
    ResetCurveEditorPoints(curveEditorPointsZ);
}

VelocityAddon::VelocityAddon(const rapidjson::Value& initialState, ParticleEmitter* owner)
    : ParticleAddon(initialState, owner)
{
    ResetCurveEditorPoints(curveEditorPointsX);
    ResetCurveEditorPoints(curveEditorPointsY);
    ResetCurveEditorPoints(curveEditorPointsZ);

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

    if (initialState.HasMember("speedXInterpolation"))
        speedXInterpolation = ParticleInterpolationType(initialState["speedXInterpolation"].GetInt());
    if (initialState.HasMember("speedYInterpolation"))
        speedYInterpolation = ParticleInterpolationType(initialState["speedYInterpolation"].GetInt());
    if (initialState.HasMember("speedZInterpoaltion"))
        speedZInterpoaltion = ParticleInterpolationType(initialState["speedZInterpoaltion"].GetInt());

    if (initialState.HasMember("MaxCurveEditorPoints"))
    {
        int minMaxPoints = initialState["MaxCurveEditorPoints"].GetInt();
        minMaxPoints     = minMaxPoints < MaxCurveEditorPoints ? minMaxPoints : MaxCurveEditorPoints;

        if (initialState.HasMember("curveEditorPointsX"))
        {
            const rapidjson::Value& dataArray = initialState["curveEditorPointsX"];

            int currentPoint                  = 0;
            for (int i = 1; i < (minMaxPoints * 2); i += 2, currentPoint++)
            {
                curveEditorPointsX[currentPoint].x = dataArray[i - 1].GetFloat();
                curveEditorPointsX[currentPoint].y = dataArray[i].GetFloat();
            }
        }

        if (initialState.HasMember("curveEditorPointsY"))
        {
            const rapidjson::Value& dataArray = initialState["curveEditorPointsY"];

            int currentPoint                  = 0;
            for (int i = 1; i < (minMaxPoints * 2); i += 2, currentPoint++)
            {
                curveEditorPointsY[currentPoint].x = dataArray[i - 1].GetFloat();
                curveEditorPointsY[currentPoint].y = dataArray[i].GetFloat();
            }
        }

        if (initialState.HasMember("curveEditorPointsZ"))
        {
            const rapidjson::Value& dataArray = initialState["curveEditorPointsZ"];

            int currentPoint                  = 0;
            for (int i = 1; i < (minMaxPoints * 2); i += 2, currentPoint++)
            {
                curveEditorPointsZ[currentPoint].x = dataArray[i - 1].GetFloat();
                curveEditorPointsZ[currentPoint].y = dataArray[i].GetFloat();
            }
        }
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

    targetState.AddMember("speedXInterpolation", (int)speedXInterpolation, allocator);
    targetState.AddMember("speedYInterpolation", (int)speedYInterpolation, allocator);
    targetState.AddMember("speedZInterpoaltion", (int)speedZInterpoaltion, allocator);

    targetState.AddMember("MaxCurveEditorPoints", MaxCurveEditorPoints, allocator);

    rapidjson::Value xEditorCurveSave(rapidjson::kArrayType);
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        xEditorCurveSave.PushBack(curveEditorPointsX[i].x, allocator).PushBack(curveEditorPointsX[i].y, allocator);
    }
    targetState.AddMember("curveEditorPointsX", xEditorCurveSave, allocator);

    rapidjson::Value yEditorCurveSave(rapidjson::kArrayType);
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        yEditorCurveSave.PushBack(curveEditorPointsY[i].x, allocator).PushBack(curveEditorPointsY[i].y, allocator);
    }
    targetState.AddMember("curveEditorPointsY", yEditorCurveSave, allocator);

    rapidjson::Value zEditorCurveSave(rapidjson::kArrayType);
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        zEditorCurveSave.PushBack(curveEditorPointsZ[i].x, allocator).PushBack(curveEditorPointsZ[i].y, allocator);
    }
    targetState.AddMember("curveEditorPointsZ", zEditorCurveSave, allocator);
}

void VelocityAddon::Init(EmitterInstance* emitterInstance)
{
    for (auto& particle : emitterInstance->particles)
    {
        InitializeParticleVelocity(particle);
    }
}

void VelocityAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
    if (!IsEnabled()) return;

    for (int i = 0; i <= emitterInstance->particleVectorPos; ++i)
    {
        Particle& particle      = emitterInstance->particles[i];
        float valueOverLifetime = particle.currentLifetime / particle.lifeTime;

        UpdateParticleVelocity(particle, valueOverLifetime);

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
        if (ImGui::BeginCombo("X Velocity type", InterpolationAddonStrings[(int)speedXInterpolation]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) speedXInterpolation = ParticleInterpolationType(i);
            }
            ImGui::EndCombo();
        }

        switch (speedXInterpolation)
        {
        case ParticleInterpolationType::FIXED_VALUES:
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
            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            ImGui::Bezier("xBezier", bezierX);
            ImGui::InputFloat2("X Range", &xSpeed[0]);
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            ImGui::Curve(
                "X Vel. Curve", ImVec2(400, 100), MaxCurveEditorPoints, curveEditorPointsX, &curveEditorIndexX,
                ImVec2(0.f, xSpeed.x), ImVec2(1.f, xSpeed.y)
            );

            ImGui::InputFloat2("X Vel. Range", &xSpeed[0]);
            ImGui::SameLine();
            if (ImGui::Button("Reset Points##XVel")) ResetCurveEditorPoints(curveEditorPointsX);
            break;
        }
        default:
            break;
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Y Speed"))
    {
        if (ImGui::BeginCombo("Y Velocity type", InterpolationAddonStrings[(int)speedYInterpolation]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) speedYInterpolation = ParticleInterpolationType(i);
            }
            ImGui::EndCombo();
        }

        switch (speedYInterpolation)
        {
        case ParticleInterpolationType::FIXED_VALUES:
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
            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            ImGui::Bezier("yBezier", bezierY);
            ImGui::InputFloat2("Y Range", &ySpeed[0]);
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            ImGui::Curve(
                "Y Vel. Curve", ImVec2(400, 100), MaxCurveEditorPoints, curveEditorPointsY, &curveEditorIndexY,
                ImVec2(0.f, ySpeed.x), ImVec2(1.f, ySpeed.y)
            );

            ImGui::InputFloat2("Y Vel. Range", &ySpeed[0]);
            ImGui::SameLine();
            if (ImGui::Button("Reset Points##YVel")) ResetCurveEditorPoints(curveEditorPointsY);
            break;
        }
        default:
            break;
        }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Z Speed"))
    {
        if (ImGui::BeginCombo("Z Velocity type", InterpolationAddonStrings[(int)speedZInterpoaltion]))
        {
            for (int i = 0; i < InterpolationAddonStringsSize; ++i)
            {
                if (ImGui::Selectable(InterpolationAddonStrings[i])) speedZInterpoaltion = ParticleInterpolationType(i);
            }
            ImGui::EndCombo();
        }

        switch (speedZInterpoaltion)
        {
        case ParticleInterpolationType::FIXED_VALUES:
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
            break;
        }
        case ParticleInterpolationType::BEZIER_SINGLE:
        {
            ImGui::Bezier("zBezier", bezierZ);
            ImGui::InputFloat2("Z Range", &zSpeed[0]);
            break;
        }
        case ParticleInterpolationType::CURVE_EDITOR:
        {
            ImGui::Curve(
                "Z Vel. Curve", ImVec2(400, 100), MaxCurveEditorPoints, curveEditorPointsZ, &curveEditorIndexZ,
                ImVec2(0.f, zSpeed.x), ImVec2(1.f, zSpeed.y)
            );

            ImGui::InputFloat2("Z Vel. Range", &zSpeed[0]);
            ImGui::SameLine();
            if (ImGui::Button("Reset Points##ZVel")) ResetCurveEditorPoints(curveEditorPointsZ);
            break;
        }
        default:
            break;
        }
    }

    // RENDER EDITOR ENDS

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void VelocityAddon::Duplicate(ParticleAddon* reference)
{
    VelocityAddon* other = reinterpret_cast<VelocityAddon*>(reference);

    if (other)
    {
        randomizeXSpeed = other->randomizeXSpeed;
        randomizeYSpeed = other->randomizeYSpeed;
        randomizeZSpeed = other->randomizeZSpeed;

        xSpeed          = other->xSpeed;
        ySpeed          = other->ySpeed;
        zSpeed          = other->zSpeed;

        for (int i = 0; i < 5; ++i)
        {
            bezierX[i] = other->bezierX[i];
            bezierY[i] = other->bezierY[i];
            bezierZ[i] = other->bezierZ[i];
        }

        speedXInterpolation = other->speedXInterpolation;
        speedYInterpolation = other->speedYInterpolation;
        speedZInterpoaltion = other->speedZInterpoaltion;

        for (int i = 0; i < MaxCurveEditorPoints; ++i)
        {
            curveEditorPointsX[i] = other->curveEditorPointsX[i];
            curveEditorPointsY[i] = other->curveEditorPointsY[i];
            curveEditorPointsZ[i] = other->curveEditorPointsZ[i];
        }

        gravity      = other->gravity;
        gravityValue = other->gravityValue;
    }
}

void VelocityAddon::ResetCurveEditorPoints(ImVec2* pointsToReset)
{
    for (int i = 0; i < MaxCurveEditorPoints; ++i)
    {
        pointsToReset[i].x = (float)i / 10.f;
        pointsToReset[i].y = (float)i / 10.f;
    }

    pointsToReset[0].x = ImGui::CurveTerminator;
}

void VelocityAddon::InitializeParticleVelocity(Particle& particle)
{
    float finalX = 0;
    float finalY = 0;
    float finalZ = 0;

    switch (speedXInterpolation)
    {
    case ParticleInterpolationType::FIXED_VALUES:
        finalX = randomizeXSpeed ? rng->Float(xSpeed.x, xSpeed.y) : xSpeed.y;
        break;
    case ParticleInterpolationType::BEZIER_SINGLE:
        finalX = xSpeed[0];
        break;
    case ParticleInterpolationType::CURVE_EDITOR:
        finalX = ImGui::CurveValue(0.f, MaxCurveEditorPoints, curveEditorPointsX);
        break;
    default:
        break;
    }

    switch (speedYInterpolation)
    {
    case ParticleInterpolationType::FIXED_VALUES:
        finalY = randomizeYSpeed ? rng->Float(ySpeed.x, ySpeed.y) : ySpeed.y;
        break;
    case ParticleInterpolationType::BEZIER_SINGLE:
        finalY = ySpeed[0];
        break;
    case ParticleInterpolationType::CURVE_EDITOR:
        finalY = ImGui::CurveValue(0.f, MaxCurveEditorPoints, curveEditorPointsY);
        break;
    default:
        break;
    }

    switch (speedZInterpoaltion)
    {
    case ParticleInterpolationType::FIXED_VALUES:
        finalZ = randomizeZSpeed ? rng->Float(zSpeed.x, zSpeed.y) : zSpeed.y;
        break;
    case ParticleInterpolationType::BEZIER_SINGLE:
        finalZ = zSpeed[0];
        break;
    case ParticleInterpolationType::CURVE_EDITOR:
        finalZ = ImGui::CurveValue(0.f, MaxCurveEditorPoints, curveEditorPointsZ);
        break;
    default:
        break;
    }

    particle.velocity = float3(finalX, finalY, finalZ);
}

void VelocityAddon::UpdateParticleVelocity(Particle& particle, float valueOverLifetime)
{
    switch (speedXInterpolation)
    {
    case ParticleInterpolationType::BEZIER_SINGLE:
        particle.velocity.x = Interpolation::Lerp(xSpeed[0], xSpeed[1], ImGui::BezierValue(valueOverLifetime, bezierX));
        break;
    case ParticleInterpolationType::CURVE_EDITOR:
        particle.velocity.x = ImGui::CurveValue(valueOverLifetime, MaxCurveEditorPoints, curveEditorPointsX);
        break;
    default:
        break;
    }

    switch (speedYInterpolation)
    {
    case ParticleInterpolationType::BEZIER_SINGLE:
        particle.velocity.y = Interpolation::Lerp(ySpeed[0], ySpeed[1], ImGui::BezierValue(valueOverLifetime, bezierY));
        break;
    case ParticleInterpolationType::CURVE_EDITOR:
        particle.velocity.y = ImGui::CurveValue(valueOverLifetime, MaxCurveEditorPoints, curveEditorPointsY);
        break;
    default:
        break;
    }

    switch (speedZInterpoaltion)
    {
    case ParticleInterpolationType::BEZIER_SINGLE:
        particle.velocity.z = Interpolation::Lerp(zSpeed[0], zSpeed[1], ImGui::BezierValue(valueOverLifetime, bezierZ));
        break;
    case ParticleInterpolationType::CURVE_EDITOR:
        particle.velocity.z = ImGui::CurveValue(valueOverLifetime, MaxCurveEditorPoints, curveEditorPointsZ);
        break;
    default:
        break;
    }
}
