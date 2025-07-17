#pragma once

#include "Script.h"

#include "Math/float2.h"

class MovingUVPostScript : public Script
{
  public:
    MovingUVPostScript(GameObject* parent) : Script(parent) {};
    ~MovingUVPostScript() override;

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
};
