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

    if (ImGui::TreeNode("Spline"))
    {
        ImGui::Text("Points: %zu", points.size());
        ImGui::DragFloat("Tension", &tension, 0.01f, 0.0f, 1.0f);

        if (ImGui::Button("Add Point")) points.push_back(float3::zero);

        ImGui::TreePop();
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
