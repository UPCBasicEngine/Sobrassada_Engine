#pragma once
#include "Component.h"

#include "Math/float3.h"
#include "Math/Quat.h"
#include <vector>

struct SplinePoint
{
    float3 position;
    Quat rotation;

    SplinePoint() : position(float3::zero), rotation(Quat::identity){}
    SplinePoint(const float3& p) : position(p), rotation(Quat::identity) {}
    SplinePoint(const float3& p, Quat r) : position(p), rotation(r){}
};

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
    void InsertPoint(const size_t i, const float3& p);
    void RemovePoint(const size_t i);
    void ClearPoints();
    float GetT(const float3& p0, const float3& p1, float tPrev) const;
    float3 CatmullRom(const float3& p0, const float3& p1, const float3& p2, const float3& p3, float segmentT) const;
    size_t Wrap(int i) const;
    float3 EvaluateSegment(const size_t seg, float segmentT) const;
    float3 Evaluate(float t) const;
    Quat EvaluateRotation(float t) const;
    void EvaluateTransform(float t, float3& pos, Quat& rot) const;
    bool PointGizmo(size_t idx);

    size_t GetNumPoints() const { return points.size(); }
    const float3 GetPointLocal(size_t idx) const { return points[idx].position; }
    float3 GetPointWorld(size_t idx) const;
    float3 GetWorldPositionInSpine(float posT) const;
    bool IsLoop() const { return loop; }

    void SetPointWorld(size_t idx, const float3& worldPos);
    void SetInWorld(bool isInWorld) { inWorld = isInWorld; }

  private:
    std::vector<SplinePoint> points;
    float3 pendingPoint  = float3::zero;

    float alpha          = 0.5f;
    bool loop            = false;

    int selectedIdx      = -1;
    const int stepsDebug = 32;

    // Debug travel marker
    bool showMarker      = false;
    float markerT        = 0.0f;
    bool inWorld         = true;
};
