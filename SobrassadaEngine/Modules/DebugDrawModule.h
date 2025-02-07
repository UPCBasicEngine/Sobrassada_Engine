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
    void Render2DLines(const std::vector<float4> &lines, const float3 &color, float depth);

  private:
    static DDRenderInterfaceCoreGL *implementation;
};
