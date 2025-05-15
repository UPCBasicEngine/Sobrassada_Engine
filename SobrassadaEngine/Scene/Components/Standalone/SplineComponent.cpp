#include "SplineComponent.h"

#include "GameObject.h"
#include "imgui.h"

SplineComponent::SplineComponent(UID uid, GameObject* parent) : Component(uid, parent, "Spline", COMPONENT_SPLINE)
{
}

SplineComponent::SplineComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Tension")) tension = initialState["Tension"].GetFloat();
    if (initialState.HasMember("Points"))
    {
        const auto& arrayPoints = initialState["Points"].GetArray();
        for (auto& p : arrayPoints)
            points.emplace_back(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat());
    }
}

SplineComponent::~SplineComponent()
{
}

void SplineComponent::Update(float deltaTime)
{
}

void SplineComponent::Render(float deltaTime)
{
}

void SplineComponent::RenderDebug(float deltaTime)
{
}

void SplineComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    if (ImGui::TreeNodeEx("Points", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t i = 0; i < points.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selectedIdx == (int)i) flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::TreeNodeEx("##point", flags, "Point %zu", i);

            if (ImGui::IsItemClicked()) selectedIdx = (int)i;

            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    ImGui::SeparatorText("Modify Point");

    if (selectedIdx >= 0 && selectedIdx < (int)points.size())
    {
        float3& p = points[selectedIdx];
        ImGui::InputFloat3("Selected Pos", &p[0]);
    }

    ImGui::SeparatorText("Properties");

    ImGui::Text("Total Points: %zu", points.size());
    ImGui::DragFloat("Tension", &tension, 0.01f, 0.0f, 1.0f);
    ImGui::Checkbox("Loop", &loop);

    ImGui::SeparatorText("Add Point");

    ImGui::InputFloat3("##newPoint", &pendingPoint[0]);
    if (ImGui::Button("Add Point"))
    {
        if (selectedIdx >= 0 && selectedIdx < (int)points.size())
            points.insert(points.begin() + selectedIdx + 1, pendingPoint);
        else points.push_back(pendingPoint);
    }
}

void SplineComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
    targetState.AddMember("Tension", tension, allocator);
    rapidjson::Value arr(rapidjson::kArrayType);

    for (const auto& p : points)
    {
        rapidjson::Value pArr(rapidjson::kArrayType);
        pArr.PushBack(p.x, allocator).PushBack(p.y, allocator).PushBack(p.z, allocator);
        arr.PushBack(pArr, allocator);
    }

    targetState.AddMember("Points", arr, allocator);
}

void SplineComponent::Clone(const Component* other)
{
    if (other->GetType() != COMPONENT_SPLINE) return;

    const SplineComponent* otherSpline = static_cast<const SplineComponent*>(other);
    points                             = otherSpline->points;
    tension                            = otherSpline->tension;
}

void SplineComponent::AddPoint(const float3& p)
{
    points.push_back(p);
}

void SplineComponent::InsertPoint(size_t i, const float3& p)
{
    points.insert(points.begin() + i, p);
}

void SplineComponent::RemovePoint(size_t i)
{
    if (i < points.size()) points.erase(points.begin() + i);
}
