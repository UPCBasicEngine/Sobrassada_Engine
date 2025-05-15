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
    void RenderEditorInspector() override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void AddPoint(const float3& p);
    void InsertPoint(size_t i, const float3& p);
    void RemovePoint(size_t i);
    
    size_t GetNumPoints() const { return points.size(); }
    const float3 GetPoint(size_t i) const { return points[i]; }

  private:
    std::vector<float3> points;
    float3 pendingPoint = float3::zero;

    float tension = 0.5f;

    bool loop     = false;

    int selectedIdx = -1;
};
