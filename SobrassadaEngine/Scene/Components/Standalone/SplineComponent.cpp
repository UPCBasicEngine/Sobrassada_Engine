#include "SplineComponent.h"

#include "GameObject.h"
#include "Math/MathFunc.h"
#include "imgui.h"

SplineComponent::SplineComponent(UID uid, GameObject* parent) : Component(uid, parent, "Spline", COMPONENT_SPLINE)
{
}

SplineComponent::SplineComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    if (initialState.HasMember("Alpha")) alpha = initialState["Alpha"].GetFloat();
    if (initialState.HasMember("Loop")) loop = initialState["Loop"].GetBool();
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
    ImGui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f);
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

    targetState.AddMember("Alpha", alpha, allocator);
    targetState.AddMember("Loop", loop, allocator);

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
    alpha                              = otherSpline->alpha;
    loop                               = otherSpline->loop;
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

float SplineComponent::GetT(const float3& p0, const float3& p1, float tPrev) const
{
    float distance = (p1 - p0).Length();
    return tPrev + powf(distance, alpha);
}

float3
SplineComponent::CatmullRom(const float3& p0, const float3& p1, const float3& p2, const float3& p3, float segmentT) const
{
    float t0  = 0.0f;
    float t1  = GetT(p0, p1, t0);
    float t2  = GetT(p1, p2, t1);
    float t3  = GetT(p2, p3, t2);

    float t   = Lerp(t1, t2, segmentT);

    float3 A1 = float3::Lerp(p0, p1, (t - t0) / (t1 - t0)); //( t1-t )/( t1-t0 )*p0 + ( t-t0 )/( t1-t0 )*p1
    float3 A2 = float3::Lerp(p1, p2, (t - t1) / (t2 - t1));
    float3 A3 = float3::Lerp(p2, p3, (t - t2) / (t3 - t2));

    float3 B1 = float3::Lerp(A1, A2, (t - t0) / (t2 - t0));
    float3 B2 = float3::Lerp(A2, A3, (t - t1) / (t3 - t1));

    return float3::Lerp(B1, B2, (t - t1) / (t2 - t1));
}

size_t SplineComponent::Wrap(int i) const
{
    int n = (int)points.size();
    return (size_t)((i % n + n) % n);
}

float3 SplineComponent::EvaluateSegment(size_t seg, float segmentT) const
{
    auto idx = [this](int k) 
    { 
        return loop ? Wrap(k) : (size_t)std::clamp(k, 0, (int)points.size() - 1); 
    };
    
    return CatmullRom(
        points[idx((int)seg - 1)], 
        points[idx((int)seg)], 
        points[idx((int)seg + 1)], 
        points[idx((int)seg + 2)],
        segmentT);

}

float3 SplineComponent::Evaluate(float t) const
{
    if (points.size() < 2) return float3::zero;

    int maxSeg = loop ? (int)points.size() : (int)points.size() - 3;
    int seg    = (int)floorf(t);
    float u    = t - seg;

    if (loop) seg = (int)Wrap(seg);
    else seg = std::clamp(seg, 0, maxSeg - 1);

    return EvaluateSegment((size_t)seg, u);
}
