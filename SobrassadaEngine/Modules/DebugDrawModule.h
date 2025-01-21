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

  private:
    static DDRenderInterfaceCoreGL *implementation;
};
