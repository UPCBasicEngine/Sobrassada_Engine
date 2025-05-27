#pragma once

#include "Component.h"
#include "Math/float3.h"
#include "Math/float4.h"
#include "imgui_color_gradient.h"
#include <deque>

class SplineComponent;

struct TrailPoint
{
    float3 position;
    float3 perpendicular;
    float time;
};

struct TrailVertex
{
    float3 position;
    float4 color;
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
    std::vector<TrailVertex> vertices;
    std::vector<int> indices;

    float minDistance = 0.5f;
    float lifeTime    = 0.5f;
    float width       = 0.1f;

    bool invertCurve  = false;
    float curve[5]    = {0.0f, 0.0f, 1.0f, 1.0f}; // Last value is an internal value for imgui

    ImGradient gradient;
    ImGradientMark* draggingMark = nullptr;
    ImGradientMark* selectedMark = nullptr;

    SplineComponent* spline;

    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;

    int maxVertices  = 64;
    int maxIndices   = 384;
};