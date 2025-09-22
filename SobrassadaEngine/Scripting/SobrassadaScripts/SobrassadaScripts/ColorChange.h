#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;

class ColorChange : public Script
{
  public:
    ColorChange(GameObject* parent);
    ~ColorChange() override;

    bool Init() override;
    void Update(float deltaTime) override {};
    void Render(float deltaTime, CameraComponent* cameraComp) override;

  private:
    unsigned int shaderProgram = 0;

    unsigned int vao           = 0;
    unsigned int vbo           = 0;
    unsigned int ebo           = 0;

    unsigned int indexCount    = 0;

    float3 targetColor         = float3::one;
    MeshComponent* meshComp    = nullptr;
};
