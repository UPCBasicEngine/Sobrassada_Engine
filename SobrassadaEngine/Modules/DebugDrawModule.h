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
    ~DebugDrawModule();

    bool Init() override;
    update_status Render(float deltaTime) override;
    bool ShutDown() override;

    void Draw(const float4x4 &view, const float4x4 &proj, unsigned width, unsigned height);
    void RenderQuadtreeViewportLines(
        const float4x4 &viewMatrix, const float4x4 &projectionMatrix, int width, int height,
        const std::vector<float4> &lines, const std::vector<float4> &elementLines, const std::vector<float4> &queryArea
    );

  private:
    static DDRenderInterfaceCoreGL *implementation;
};
