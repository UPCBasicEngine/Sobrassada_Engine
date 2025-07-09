#pragma once

#include "Script.h"

#include "Math/float2.h"

class GBuffer;

class RenderTestScript : public Script
{
  public:
    RenderTestScript(GameObject* parent) : Script(parent) {};
    ~RenderTestScript() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

  private:
    unsigned int shaderProgram = 0;

    unsigned int vao           = 0;
    unsigned int vbo           = 0;
    unsigned int ebo           = 0;

    unsigned int texture       = 0;

    unsigned int indexCount    = 0;

    float animationSpeed       = 0.1f;
    float2 uvOffset            = float2::zero;

    GBuffer* gbuffer           = nullptr;
};
