#include "VolumetricAreaComponent.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "SceneModule.h"
#include "GameObject.h"

#include "Geometry/AABB.h"
#include "Geometry/LineSegment.h"
#include "ImGui.h"

#include <vector>

VolumetricAreaComponent::VolumetricAreaComponent(UID uid, GameObject* parent)
    : Component(uid, parent, "Volumetric area", COMPONENT_VOLUMETRIC_AREA)
{
}

VolumetricAreaComponent::VolumetricAreaComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("volumeType")) volumeType = VolumetricAreaType(initialState["volumeType"].GetInt());

    if (initialState.HasMember("size"))
    {
        const rapidjson::Value& dataArray = initialState["size"];
        size                              = {dataArray[0].GetFloat(), dataArray[1].GetFloat(), dataArray[2].GetFloat()};
    }
}

VolumetricAreaComponent::~VolumetricAreaComponent()
{
}

void VolumetricAreaComponent::Init()
{
}

void VolumetricAreaComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);

    targetState.AddMember("volumeType", (int)volumeType, allocator);

    rapidjson::Value sizeSave(rapidjson::kArrayType);
    sizeSave.PushBack(size.x, allocator).PushBack(size.y, allocator).PushBack(size.z, allocator);
    targetState.AddMember("size", sizeSave, allocator);
}

void VolumetricAreaComponent::Clone(const Component* other)
{

}

void VolumetricAreaComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (ImGui::BeginCombo("Volume type", VolumetricAreaTypeStrings[(int)volumeType]))
    {
        for (int i = 0; i < VolumetricAreaTypeStringsSize; ++i)
        {
            if (ImGui::Selectable(VolumetricAreaTypeStrings[i]))
            {
                volumeType = VolumetricAreaType(i);
            }
        }
        ImGui::EndCombo();
    }

    switch (volumeType)
    {
    case VolumetricAreaType::BOX:
    {
        ImGui::DragFloat3("AABB Sizes", &size[0], 0.01f, 1.0f, 100.f);
        break;
    }
    case VolumetricAreaType::SPHERE:
    {
        ImGui::DragFloat("Sphere radius", &size[0], 0.01f, 1.0f, 100.f);
        break;
    }
    default:
    {
        ImGui::Text("SOMETHING IS WRONG WITH AREA TYPE!");
        break;
    }
    }
}

void VolumetricAreaComponent::Update(float deltaTime)
{
}

void VolumetricAreaComponent::RenderDebug(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (App->GetSceneModule()->GetInPlayMode()) return;

    DebugDrawModule* debug = App->GetDebugDrawModule();

    switch (volumeType)
    {
    case VolumetricAreaType::BOX:
    {
        float3 globalPos = parent->GetGlobalPostition();

        AABB temp        = AABB(globalPos - (size / 2.0), globalPos + (size / 2.0));

        std::vector<LineSegment> edges;
        edges.reserve(temp.NumEdges());

        for (int i = 0; i < temp.NumEdges(); ++i)
        {
            edges.push_back(temp.Edge(i));
        }

        debug->RenderLines(edges, VolumetricAreaDebugColor);
        break;
    }
    case VolumetricAreaType::SPHERE:
    {
        debug->DrawSphere(parent->GetGlobalPostition(), VolumetricAreaDebugColor, size.x);
        break;
    }
    default:
    {
        ImGui::Text("SOMETHING IS WRONG WITH AREA TYPE!");
        break;
    }
    }
}
