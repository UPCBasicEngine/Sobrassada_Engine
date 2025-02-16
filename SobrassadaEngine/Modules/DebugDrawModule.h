#pragma once

#include "Module.h"

#include "Math/float4x4.h"
#include <list>

class DDRenderInterfaceCoreGL;
class Camera;

class DebugDrawModule : public Module
{
  public:
    DebugDrawModule();
    ~DebugDrawModule() override;

    bool Init() override;
    update_status Render(float deltaTime) override;
    bool ShutDown() override;

    void Draw();
    void Draw(const float4x4 &view, const float4x4 &proj, unsigned width, unsigned height);
    void Render2DLines(const std::vector<float4> &lines, const float3 &color, float depth);
    void RenderLines(const std::vector<LineSegment> &lines, const float3& color);
    void DrawLine(const float3 &origin, const float3 &direction, const float distance, const float3 &color);
    void DrawCircle(const float3 &center, const float3 &upVector, const float3 &color, const float radius);
    void DrawSphere(const float3 &center, const float3 &color, const float radius);

  private:
    static DDRenderInterfaceCoreGL *implementation;
};
