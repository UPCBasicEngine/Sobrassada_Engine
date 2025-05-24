#pragma once
#include "Component.h"

#include <vector>
#include "Math/float3.h"


class SOBRASADA_API_ENGINE SplineComponent : public Component
{
public:

    SplineComponent(UID uid, GameObject* parent);
    SplineComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~SplineComponent() override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    bool RenderGizmo() override;
    void RenderEditorInspector() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void AddPoint(const float3& p);
    void InsertPoint(size_t i, const float3& p);
    void RemovePoint(size_t i);
    float GetT(const float3& p0, const float3& p1, float tPrev) const;
    float3 CatmullRom(
        const float3& p0, const float3& p1, const float3& p2, const float3& p3, float segmentT
    ) const;
    size_t Wrap(int i) const;
    float3 EvaluateSegment(const size_t seg, float segmentT) const;
    float3 Evaluate(float t) const;
    bool PointGizmo(size_t idx);

    size_t GetNumPoints() const { return points.size(); }
    const float3 GetPointLocal(size_t idx) const { return points[idx]; }
    float3 GetPointWorld(size_t idx) const;
    float3 GetWorldPositionInSpine(float posT) const;

    void SetPointWorld(size_t idx, const float3& worldPos);

  private:
    std::vector<float3> points;
    float3 pendingPoint = float3::zero;

    float alpha = 0.5f;
    bool loop     = false;

    int selectedIdx = -1;
    const int stepsDebug      = 16;

    // Debug travel marker
    bool showMarker           = false;
    float markerT             = 0.0f;
};
