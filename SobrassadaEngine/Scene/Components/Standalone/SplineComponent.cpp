#include "SplineComponent.h"

#include "Application.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "Geometry/LineSegment.h"
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
        {
            if (p.IsObject() && p.HasMember("Pos") && p.HasMember("Rot"))
            {
                const auto& pos = p["Pos"].GetArray();
                const auto& rot = p["Rot"].GetArray();

                float3 position(pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat());
                Quat rotation(rot[0].GetFloat(), rot[1].GetFloat(), rot[2].GetFloat(), rot[3].GetFloat()); // w,x,y,z

                points.emplace_back(position, rotation);
            }
            else if (p.IsArray() && p.Size() == 3) //Old format (array x,y,z)
            {
                points.emplace_back(float3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat()), Quat::identity);
            }
        }
    }
}

SplineComponent::~SplineComponent()
{
}

void SplineComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void SplineComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
}

void SplineComponent::RenderDebug(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (points.size() < 2) return;

    EditorUIModule* ui   = App->GetEditorUIModule();
    DebugDrawModule* dbg = App->GetDebugDrawModule();

    const size_t endSeg        = loop ? points.size() : points.size() - 1;
    const float3 curveColor(0, 1, 0); // Green
    const float3 pointColor(1, 0, 0); // Red

    const float3 worldOffset = inWorld ? parent->GetGlobalTransform().TranslatePart() : float3(0, 0, 0);

    auto drawLine = [&](const float3& a, const float3& b) { dbg->DrawLineSegment(LineSegment(a, b), curveColor); };

    for (size_t seg = 0; seg < endSeg; ++seg)
    {
        if (loop || points.size() >= 3)
        {
            float3 prev = EvaluateSegment(seg, 0.0f) + worldOffset;

            for (int i = 1; i <= stepsDebug; ++i)
            {
                const float u  = (float)i / stepsDebug;
                const float3 p = EvaluateSegment(seg, u) + worldOffset;
                drawLine(prev, p);
                prev = p;
            }
        }
        else drawLine(points[seg].position, points[seg + 1].position);
    }

    for (const SplinePoint& p : points)
        dbg->DrawSphere(p.position + worldOffset, pointColor, 0.08f);

    if (showMarker && points.size() >= 2)
    {
        const float3 wPos = GetWorldPositionInSpine(markerT);
        dbg->DrawSphere(wPos, float3(1, 1, 0), 0.10f);
    }
}


bool SplineComponent::RenderGizmo()
{
    return PointGizmo((size_t)selectedIdx);
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

    const bool validSel = selectedIdx >= 0 && selectedIdx < (int)points.size();

    if (validSel)
    {
        float3 tempPoint = points[selectedIdx].position;
        if (ImGui::InputFloat3("Selected Pos", &tempPoint[0])) points[selectedIdx] = tempPoint;

        float3 eulerDeg = points[selectedIdx].rotation.ToEulerXYZ() * RAD_DEGREE_CONV;
        if (ImGui::InputFloat3("Selected Rot (deg)", &eulerDeg[0]))
            points[selectedIdx].rotation = Quat::FromEulerXYZ(
                eulerDeg.x * DEGREE_RAD_CONV, eulerDeg.y * DEGREE_RAD_CONV, eulerDeg.z * DEGREE_RAD_CONV
            );
    }

    ImGui::BeginDisabled(!validSel);
    if (ImGui::Button("Delete Point") && validSel)
    {
        points.erase(points.begin() + selectedIdx);
        if (points.empty()) selectedIdx = -1;
        else if (selectedIdx >= (int)points.size()) selectedIdx = (int)points.size() - 1;
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("Properties");

    ImGui::Text("Total Points: %zu", points.size());
    ImGui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f);
    ImGui::Checkbox("Loop", &loop);

    ImGui::SeparatorText("Path Probe");

    ImGui::Checkbox("Show marker", &showMarker);
    if (showMarker) ImGui::DragFloat("t  (0-1)", &markerT, 0.001f, 0.f, 1.f, "%.3f");

    ImGui::SeparatorText("Add Point");

    static bool pendingInsert = false;
    static int insertAfter    = -1;

    ImGui::InputFloat3("##newPoint", &pendingPoint[0]);
    if (ImGui::Button("Add Point"))
    {
        pendingInsert = true;
        insertAfter   = selectedIdx;
    }

    if (pendingInsert)
    {
        if (insertAfter >= 0 && insertAfter < (int)points.size())
            points.insert(points.begin() + insertAfter + 1, pendingPoint);
        else points.push_back(pendingPoint);

        selectedIdx   = insertAfter + 1;
        pendingInsert = false;
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
        rapidjson::Value pObj(rapidjson::kObjectType);
        //position
        rapidjson::Value posArr(rapidjson::kArrayType);
        posArr.PushBack(p.position.x, allocator).PushBack(p.position.y, allocator).PushBack(p.position.z, allocator);
        pObj.AddMember("Pos", posArr, allocator);
        //rotation
        rapidjson::Value rotArr(rapidjson::kArrayType);
        rotArr.PushBack(p.rotation.x, allocator)
            .PushBack(p.rotation.y, allocator)
            .PushBack(p.rotation.z, allocator)
            .PushBack(p.rotation.w, allocator);
        pObj.AddMember("Rot", rotArr, allocator);

        arr.PushBack(pObj, allocator);
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
    enabled                            = otherSpline->enabled;
    wasEnabled                         = otherSpline->wasEnabled;
}

void SplineComponent::AddPoint(const float3& p)
{
    points.push_back(p);
}

void SplineComponent::InsertPoint(const size_t i, const float3& p)
{
    points.insert(points.begin() + i, p);
}

void SplineComponent::RemovePoint(const size_t i)
{
    if (i < points.size())
    {
        points.erase(points.begin() + i);
        selectedIdx = -1;
    }
}

void SplineComponent::ClearPoints()
{
    selectedIdx = -1;
    points.clear();
}

float SplineComponent::GetT(const float3& p0, const float3& p1, float tPrev) const
{
    float distance = (p1 - p0).Length();

    if (distance < 0.0001f) distance = 0.0001f; // in order to be able to use CatmullRom with 3 points

    return tPrev + powf(distance, alpha);
}

float3 SplineComponent::CatmullRom(
    const float3& p0, const float3& p1, const float3& p2, const float3& p3, float segmentT
) const
{
    const float t0 = 0.0f;
    const float t1 = GetT(p0, p1, t0);
    const float t2 = GetT(p1, p2, t1);
    const float t3 = GetT(p2, p3, t2);

    const float t  = Lerp(t1, t2, segmentT);

    const float3 A1 = float3::Lerp(p0, p1, (t - t0) / (t1 - t0)); //( t1-t )/( t1-t0 )*p0 + ( t-t0 )/( t1-t0 )*p1
    const float3 A2 = float3::Lerp(p1, p2, (t - t1) / (t2 - t1));
    const float3 A3 = float3::Lerp(p2, p3, (t - t2) / (t3 - t2));

    const float3 B1 = float3::Lerp(A1, A2, (t - t0) / (t2 - t0));
    const float3 B2 = float3::Lerp(A2, A3, (t - t1) / (t3 - t1));

    return float3::Lerp(B1, B2, (t - t1) / (t2 - t1));
}

size_t SplineComponent::Wrap(int i) const
{
    const int n = (int)points.size();
    return (size_t)((i % n + n) % n);
}

float3 SplineComponent::EvaluateSegment(const size_t seg, float segmentT) const
{
    auto idx = [this](int k)
    {
        const int n = (int)points.size();
        if (loop) return Wrap(k);

        if (k < 0) return (size_t)0;
        if (k >= n) return (size_t)(n - 1);
        return (size_t)k;
    };

    return CatmullRom(
        points[idx((int)seg - 1)].position, points[idx((int)seg)].position, points[idx((int)seg + 1)].position,
        points[idx((int)seg + 2)].position, segmentT
    );
}

float3 SplineComponent::Evaluate(float t) const
{
    const float3 worldOffset = parent->GetGlobalTransform().TranslatePart();

    if (points.size() < 2) return worldOffset;

    t = std::clamp(t, 0.f, 1.f);

    const int numSeg = loop ? (int)points.size() : (int)points.size() - 1;

    const float segFloat   = t * numSeg;
    if (segFloat >= numSeg) return worldOffset + (loop ? points.front().position : points.back().position);

    int seg = (int)floorf(segFloat);
    const float segmentT = segFloat - seg;

    if (loop) seg = seg % points.size();

    const float3 local = EvaluateSegment((size_t)seg, segmentT);
    return worldOffset + local;
}

Quat SplineComponent::EvaluateRotation(float t) const
{
    if (points.empty()) return Quat::identity;
    if (points.size() == 1) return points.front().rotation;

    t = std::clamp(t, 0.f, 1.f);

    const int numSeg = loop ? (int)points.size() : (int)points.size() - 1;
    const float segFloat = t * numSeg;

    if (segFloat >= numSeg) return loop ? points.front().rotation : points.back().rotation;

    int seg = (int)floorf(segFloat);
    float segmentT = segFloat - seg;

    if (loop) seg = seg % points.size();

    const int nextIdx = loop ? (seg + 1) % points.size() : std::min(seg + 1, (int)points.size() - 1);
    
    return Quat::Slerp(points[seg].rotation, points[nextIdx].rotation, segmentT).Normalized();
}

void SplineComponent::EvaluateTransform(float t, float3& pos, Quat& rot) const
{
    pos = Evaluate(t);
    rot = EvaluateRotation(t);
}



bool SplineComponent::PointGizmo(size_t idx)
{
    if (selectedIdx >= 0 && selectedIdx < (int)points.size())
    {
        float4x4 localMatrix  = float4x4::FromTRS(points[idx].position, float4x4::identity, float3::one);
        
        //In order to ignore Rotation and Scale from parent and set it to 1
        const float3 translate        = parent->GetGlobalTransform().TranslatePart();
        float4x4 parentT              = float4x4::FromTRS(translate, float4x4::identity, float3::one);

        float4x4 globalMatrix         = parentT * localMatrix;

        float3 newPos, _unusedRot, _unusedScale;

        bool moved = App->GetEditorUIModule()->RenderImGuizmo(
            localMatrix, globalMatrix, parentT, newPos, _unusedRot, _unusedScale
        );

        if (moved) points[idx] = localMatrix.TranslatePart();
        return true;
    }
    return false;
}

float3 SplineComponent::GetPointWorld(size_t idx) const
{
    if (idx >= points.size()) return float3::zero;

    return points[idx].position + parent->GetGlobalTransform().TranslatePart();
}

float3 SplineComponent::GetWorldPositionInSpine(float posT) const
{
    if (points.size() < 2) return parent->GetGlobalTransform().TranslatePart();

    return Evaluate(posT);
}

void SplineComponent::SetPointWorld(size_t idx, const float3& worldPos)
{
    if (idx >= points.size()) return;

    points[idx] = worldPos - parent->GetGlobalTransform().TranslatePart();
}
