#include "AreaAddon.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "EmitterInstance.h"
#include "GameObject.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

#include "Geometry/AABB.h"
#include "Geometry/LineSegment.h"
#include "Math/float4x4.h"
#include "Math/Quat.h"
#include "imgui.h"

#include <vector>

AreaAddon::AreaAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::AREA, owner)
{
    basicCube.minPoint = -float3::one;
    basicCube.maxPoint = float3::one;

    currentShape       = ParticleAreaShape::CUBE;
    ManageShapeSwitch(ParticleAreaShape::NONE);
}

AreaAddon::AreaAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("currentShape")) currentShape = ParticleAreaShape(initialState["currentShape"].GetInt());
    if (initialState.HasMember("currentSpawn")) currentSpawn = ParticleAreaSpawn(initialState["currentSpawn"].GetInt());

    if (initialState.HasMember("baseRadius")) baseRadius = initialState["baseRadius"].GetFloat();
    if (initialState.HasMember("coneAngle")) coneAngle = initialState["coneAngle"].GetFloat();
    if (initialState.HasMember("coneLength")) coneLength = initialState["coneLength"].GetFloat();

    if (initialState.HasMember("cubeSize"))
    {
        const rapidjson::Value& dataArray = initialState["cubeSize"];
        cubeSize = float3(dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat());
    }

    basicCube.minPoint = -float3::one;
    basicCube.maxPoint = float3::one;

    ManageShapeSwitch(ParticleAreaShape::NONE);
}

AreaAddon::~AreaAddon()
{
}

void AreaAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("currentShape", (int)currentShape, allocator);
    targetState.AddMember("currentSpawn", (int)currentSpawn, allocator);

    targetState.AddMember("baseRadius", baseRadius, allocator);
    targetState.AddMember("coneAngle", coneAngle, allocator);
    targetState.AddMember("coneLength", coneLength, allocator);

    rapidjson::Value cubeSizeSave(rapidjson::kArrayType);
    cubeSizeSave.PushBack(cubeSize.x, allocator).PushBack(cubeSize.y, allocator).PushBack(cubeSize.z, allocator);
    targetState.AddMember("cubeSize", cubeSizeSave, allocator);
}

void AreaAddon::Init(EmitterInstance* emitterInstance)
{
    const float4x4& globalTransform = emitterInstance->GetOwner()->GetParent()->GetGlobalTransform();
    UpdateShapesTransforms(globalTransform);

    for (auto& particle : emitterInstance->particles)
    {
        AssignPositionDirection(particle);
    }
}

void AreaAddon::Update(float deltaTime, EmitterInstance* emitterInstance)
{
}

void AreaAddon::RenderEditorInspector()
{
    ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Area Addon");

    if (ImGui::BeginCombo("Current shape", AreaAddonStrings[(int)currentShape]))
    {
        ParticleAreaShape previousShape = currentShape;
        for (int i = 0; i < AreaAddonStringsSize; ++i)
        {
            if (ImGui::Selectable(AreaAddonStrings[i]))
            {
                currentShape = ParticleAreaShape(i);
                ManageShapeSwitch(previousShape);
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PushItemWidth(200);

    switch (currentShape)
    {
    case ParticleAreaShape::NONE:
        break;
    case ParticleAreaShape::CUBE:
        RenderCubeEditor();
        break;
    case ParticleAreaShape::CIRCLE:
        RenderCircleEditor();
        break;
    case ParticleAreaShape::SPHERE:
        RenderSphereEditor();
        break;
    case ParticleAreaShape::CONE:
        RenderConeEditor();
        break;
    default:
        break;
    }

    ImGui::PopItemWidth();
}

void AreaAddon::RenderDebug(GameObject* parent)
{
    DebugDrawModule* debug          = App->GetDebugDrawModule();
    const float4x4& globalTransform = parent->GetGlobalTransform();

    switch (currentShape)
    {
    case ParticleAreaShape::NONE:
        break;
    case ParticleAreaShape::CUBE:
    {
        std::vector<LineSegment> edges;
        edges.assign(12, LineSegment());

        cube   = globalTransform * OBB(basicCube);
        cube.r = cubeSize;

        for (int i = 0; i < 12; ++i)
            edges[i] = cube.Edge(i);

        debug->RenderLines(edges, float3::one);

        break;
    }
    case ParticleAreaShape::CIRCLE:
    {
        circle   = globalTransform * Circle(float3::zero, float3::unitY, baseRadius);
        circle.r = baseRadius;

        debug->DrawArrow(circle.pos, circle.pos + circle.normal, float3::one, 0.2f);
        debug->DrawCircle(circle.pos, circle.normal, float3::one, circle.r);
        break;
    }
    case ParticleAreaShape::SPHERE:
    {
        sphere.pos = globalTransform.TranslatePart();
        sphere.r   = baseRadius;

        debug->DrawSphere(sphere.pos, float3::one, sphere.r);

        break;
    }
    case ParticleAreaShape::CONE:
    {
        circle   = globalTransform * Circle(float3::zero, float3::unitY, baseRadius);
        circle.r = baseRadius;

        debug->DrawCone(circle.pos, circle.normal * coneLength, float3::one, baseRadius, topRadius);

        break;
    }
    default:
        break;
    }
}

void AreaAddon::AssignPositionDirection(Particle& particle)
{
    float3 newPosition  = float3::zero;
    float3 newDirection = float3::one;

    switch (currentSpawn)
    {
    case ParticleAreaSpawn::NONE:
        break;
    case ParticleAreaSpawn::SURFACE:
    {
        if (currentShape == ParticleAreaShape::CUBE)
        {
            newPosition = cube.RandomPointOnSurface(*rng);
        }

        else if (currentShape == ParticleAreaShape::SPHERE)
        {
            newPosition  = sphere.RandomPointOnSurface(*rng);
            newDirection = newPosition - sphere.pos;
        }
        else if (currentShape == ParticleAreaShape::CIRCLE)
        {
            newPosition  = circle.RandomPointInside(*rng);
            newDirection = newPosition - circle.pos;
        }
        else if (currentShape == ParticleAreaShape::CONE)
        {
            newPosition         = circle.RandomPointInside(*rng);

            const float rx            = rng->Float(-coneAngle, coneAngle) * DEGREE_RAD_CONV;
            const float rz            = rng->Float(-coneAngle, coneAngle) * DEGREE_RAD_CONV;

            float3 tempDir    = (Quat::FromEulerXYZ(rx, 0.f, rz) * float3::unitY).Normalized();

            newDirection      = lastGlobalTransform.MulDir(tempDir).Normalized();
        }

        break;
    }
    case ParticleAreaSpawn::VOLUME:
    {
        if (currentShape == ParticleAreaShape::CUBE)
        {
            newPosition  = cube.RandomPointInside(*rng);
            newDirection = newPosition - cube.pos;
        }

        else if (currentShape == ParticleAreaShape::SPHERE)
        {
            newPosition  = sphere.RandomPointInside(*rng);
            newDirection = newPosition - sphere.pos;
        }
        break;
    }
    default:
        break;
    }

    particle.position  = newPosition;
    particle.direction = newDirection.Normalized();
}

void AreaAddon::ManageShapeSwitch(ParticleAreaShape previousShape)
{
    if (previousShape == currentShape) return;

    switch (currentShape)
    {
    case ParticleAreaShape::NONE:
        break;
    case ParticleAreaShape::CUBE:
        cube.pos     = float3::zero;
        cube.r       = cubeSize;
        cube.axis[0] = float3(1, 0, 0);
        cube.axis[1] = float3(0, 1, 0);
        cube.axis[2] = float3(0, 0, 1);

        break;
    case ParticleAreaShape::CIRCLE:
        circle = Circle(float3::zero, float3::unitY, baseRadius);
        break;
    case ParticleAreaShape::SPHERE:
        sphere = Sphere(float3::zero, baseRadius);
        break;
    case ParticleAreaShape::CONE:
        circle = Circle(float3::zero, float3::unitY, baseRadius);
        RecalculateConeTopRadius();
        break;
    default:
        break;
    }

    currentSpawn = ParticleAreaSpawn::SURFACE;
}

void AreaAddon::RenderCubeEditor()
{
    if (ImGui::DragFloat3("Cube Size", &cubeSize[0], 0.01f, 0.f, 50.f, "%.2f"))
    {
        cube.r = cubeSize;
    }

    if (ImGui::BeginCombo("Spawn location", AreaAddonSpawnStrings[(int)currentSpawn]))
    {
        for (int i = 0; i < AreaAddonSpawnStringsSize; ++i)
        {
            if (ImGui::Selectable(AreaAddonSpawnStrings[i])) currentSpawn = ParticleAreaSpawn(i);
        }
        ImGui::EndCombo();
    }
}

void AreaAddon::RenderCircleEditor()
{
    if (ImGui::DragFloat("Circle Radius", &baseRadius, 0.01f, 0.f, 50.f, "%.2f"))
    {
        circle.r = baseRadius;
    }
}

void AreaAddon::RenderSphereEditor()
{
    if (ImGui::DragFloat("Sphere Radius", &baseRadius, 0.01f, 0.f, 50.f, "%.2f"))
    {
        sphere.r = baseRadius;
    }

    if (ImGui::BeginCombo("Spawn location", AreaAddonSpawnStrings[(int)currentSpawn]))
    {
        for (int i = 0; i < AreaAddonSpawnStringsSize; ++i)
        {
            if (ImGui::Selectable(AreaAddonSpawnStrings[i])) currentSpawn = ParticleAreaSpawn(i);
        }
        ImGui::EndCombo();
    }
}

void AreaAddon::RenderConeEditor()
{
    if (ImGui::DragFloat("Base Radius", &baseRadius, 0.01f, 0.f, 50.f, "%.2f"))
    {
        circle.r = baseRadius;
        RecalculateConeTopRadius();
    }

    if (ImGui::DragFloat("Cone angle", &coneAngle, 0.05f, 0.f, 90.f, "%.2f")) RecalculateConeTopRadius();
    if (ImGui::DragFloat("Cone length", &coneLength, 0.01f, 0.f, 50.f, "%.2f")) RecalculateConeTopRadius();
}

void AreaAddon::RecalculateConeTopRadius()
{
    topRadius = baseRadius + (tan(coneAngle * DEGREE_RAD_CONV) * coneLength);
}

void AreaAddon::UpdateShapesTransforms(const float4x4& globalTransform)
{
    cube       = globalTransform * OBB(basicCube);
    cube.r     = cubeSize;

    circle     = globalTransform * Circle(float3::zero, float3::unitY, baseRadius);
    circle.r   = baseRadius;

    sphere.pos = globalTransform.TranslatePart();
    sphere.r   = baseRadius;

    lastGlobalTransform = globalTransform;
}
