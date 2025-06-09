#include "AreaAddon.h"

#include "EmitterInstance.h"
#include "GameObject.h"
#include "ParticleEmitter.h"

#include "Geometry/Circle.h"
#include "Geometry/OBB.h"
#include "Geometry/Sphere.h"
#include "imgui.h"

AreaAddon::AreaAddon(ParticleEmitter* owner) : ParticleAddon(ParticleAddonType::AREA, owner)
{
    currentShape = ParticleAreaShape::CUBE;
    ManageShapeSwitch(ParticleAreaShape::NONE);
}

AreaAddon::AreaAddon(const rapidjson::Value& initialState, ParticleEmitter* owner) : ParticleAddon(initialState, owner)
{
    if (initialState.HasMember("currentShape")) currentShape = ParticleAreaShape(initialState["currentShape"].GetInt());

    if (initialState.HasMember("baseRadius")) baseRadius = initialState["baseRadius"].GetFloat();
    if (initialState.HasMember("topRadius")) topRadius = initialState["topRadius"].GetFloat();
    if (initialState.HasMember("coneAngle")) coneAngle = initialState["coneAngle"].GetFloat();

    if (initialState.HasMember("cubeSize"))
    {
        const rapidjson::Value& dataArray = initialState["cubeSize"];
        cubeSize = float3(dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat());
    }

    ManageShapeSwitch(ParticleAreaShape::NONE);
}

AreaAddon::~AreaAddon()
{
    delete cube;
    delete sphere;
    delete circle;
}

void AreaAddon::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    ParticleAddon::Save(targetState, allocator);

    targetState.AddMember("currentShape", (int)currentShape, allocator);

    targetState.AddMember("baseRadius", baseRadius, allocator);
    targetState.AddMember("topRadius", topRadius, allocator);
    targetState.AddMember("coneAngle", coneAngle, allocator);

    rapidjson::Value cubeSizeSave(rapidjson::kArrayType);
    cubeSizeSave.PushBack(cubeSize.x, allocator).PushBack(cubeSize.y, allocator).PushBack(cubeSize.z, allocator);
    targetState.AddMember("cubeSize", cubeSizeSave, allocator);
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
        if (cube) RenderCubeEditor();
        break;
    case ParticleAreaShape::CIRCLE:
        if (circle) RenderCircleEditor();
        break;
    case ParticleAreaShape::SPHERE:
        if (sphere) RenderSphereEditor();
        break;
    case ParticleAreaShape::CONE:
        RenderConeEditor();
        break;
    default:
        break;
    }

    ImGui::PopItemWidth();
}

void AreaAddon::ManageShapeSwitch(ParticleAreaShape previousShape)
{
    if (previousShape == currentShape) return;

    switch (previousShape)
    {
    case ParticleAreaShape::NONE:
        break;
    case ParticleAreaShape::CUBE:
        delete cube;
        break;
    case ParticleAreaShape::CIRCLE:
        delete circle;
        break;
    case ParticleAreaShape::SPHERE:
        delete sphere;
        break;
    case ParticleAreaShape::CONE:
        break;
    default:
        break;
    }

    switch (currentShape)
    {
    case ParticleAreaShape::NONE:
        break;
    case ParticleAreaShape::CUBE:
        cube          = new OBB();

        cube->pos     = float3::zero;
        cube->r       = cubeSize;
        cube->axis[0] = float3(1, 0, 0);
        cube->axis[1] = float3(0, 1, 0);
        cube->axis[2] = float3(0, 0, 1);

        break;
    case ParticleAreaShape::CIRCLE:
        circle = new Circle(float3::zero, float3::unitY, baseRadius);
        break;
    case ParticleAreaShape::SPHERE:
        sphere = new Sphere(float3::zero, baseRadius);
        break;
    case ParticleAreaShape::CONE:
        break;
    default:
        break;
    }
}

void AreaAddon::RenderCubeEditor()
{
    if (ImGui::DragFloat3("Cube Size", &cubeSize[0], 0.01f, 0.f, 50.f, "%.2f"))
    {
        cube->r = cubeSize;
    }
}

void AreaAddon::RenderCircleEditor()
{
    if (ImGui::DragFloat("Circle Radius", &baseRadius, 0.01f, 0.f, 50.f, "%.2f"))
    {
        circle->r = baseRadius;
    }
}

void AreaAddon::RenderSphereEditor()
{
    if (ImGui::DragFloat("Sphere Radius", &baseRadius, 0.01f, 0.f, 50.f, "%.2f"))
    {
        sphere->r = baseRadius;
    }
}

void AreaAddon::RenderConeEditor()
{
    ImGui::Text("NOT IMPLEMENTED YET :P");
}
