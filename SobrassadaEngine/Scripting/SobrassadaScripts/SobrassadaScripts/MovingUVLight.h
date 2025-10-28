#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;

class MovingUVLight : public Script
{
  public:
    MovingUVLight(GameObject* parent);
    ~MovingUVLight() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

    void Reset() override;

  private:
    unsigned int shaderProgram       = 0;

    unsigned int vao                 = 0;
    unsigned int vbo                 = 0;
    unsigned int ebo                 = 0;

    bool isAlphaDiscard              = false;

    unsigned int indexCount          = 0;

    float animationSpeed             = 0.1f;
    float2 uvOffset                  = float2::zero;
    float2 uvOffsetStart             = float2::zero;
    float2 uvOffsetDirection         = float2::one;

    MeshComponent* meshComp          = nullptr;
};
