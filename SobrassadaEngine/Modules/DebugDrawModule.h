#pragma once

#include "Module.h"

#include "Math/float4x4.h"

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
    void RenderLines(float4x4 &viewMatrix, float4x4 &projectionMatrix, int width, int height);

  private:
    static DDRenderInterfaceCoreGL *implementation;
};
