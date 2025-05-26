#pragma once

#include "Component.h"
#include "Math/float3.h"
#include <deque>

class SplineComponent;

struct TrailPoint
{
    float3 position;
    float3 perpendicular;
    float time;
};

class TrailComponent : public Component
{
  public:
    TrailComponent(UID uid, GameObject* parent);
    TrailComponent(const rapidjson::Value& initialState, GameObject* parent);
    ~TrailComponent() override;

    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const override;
    void Clone(const Component* other) override;

    void Update(float deltaTime) override;
    void Render(float deltaTime) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void ParentUpdated() override;

    void RecalculateAABB();

  private:
    std::deque<TrailPoint> points;
    std::vector<float3> vertices;
    std::vector<int> indices;

    float minDistance = 0.5f;
    float lifeTime    = 0.5f;
    float width       = 0.1f;
    SplineComponent* spline;

    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;

    int maxVertices  = 64;
    int maxIndices   = 384;
};