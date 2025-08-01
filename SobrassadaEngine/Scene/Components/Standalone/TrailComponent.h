#pragma once

#include "Component.h"
#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"
#include <deque>

class SplineComponent;
class ResourceTexture;
class ImGradient;
struct ImGradientMark;

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
    float2 uv;
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
    void Render(float deltaTime, CameraComponent* camera) override;
    void RenderDebug(float deltaTime) override;
    void RenderEditorInspector() override;
    void ParentUpdated() override;

    void UpdateTexture(UID newTextureUID);
    void RecalculateAABB();

  private:
    std::deque<TrailPoint> points;
    std::vector<TrailVertex> vertices;
    std::vector<int> indices;

    float minDistance               = 0.5f;
    float lifeTime                  = 0.5f;
    float width                     = 0.1f;
    float cutoff                    = 0.1f;

    bool useCurve                   = false;
    bool invertCurve                = false;
    float curve[5]                  = {0.0f, 0.0f, 1.0f, 1.0f}; // Last value is an internal value for imgui

    ImGradient* gradient            = nullptr;
    ImGradientMark* draggingMark    = nullptr;
    ImGradientMark* selectedMark    = nullptr;

    bool hasTexture                 = false;
    std::string currentResourceName = "No material";
    UID currentTextureUID           = FALLBACK_TEXTURE_UID;
    ResourceTexture* currentTexture = nullptr;

    SplineComponent* spline;

    unsigned int vao     = 0;
    unsigned int vbo     = 0;
    unsigned int ebo     = 0;

    int maxVertices      = 1024;
    int maxIndices       = 3072;

    float4x4 modelMatrix = float4x4::identity;
};