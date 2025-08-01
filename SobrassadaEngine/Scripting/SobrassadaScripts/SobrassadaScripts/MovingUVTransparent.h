#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;

class MovingUVTransparent : public Script
{
  public:
    MovingUVTransparent(GameObject* parent);
    ~MovingUVTransparent() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

  private:
    unsigned int shaderProgram   = 0;

    unsigned int vao             = 0;
    unsigned int vbo             = 0;
    unsigned int ebo             = 0;
    unsigned int materialBuffer  = 0;

    bool isAlphaDiscard          = false;
    bool isDoubleSided           = false;

    unsigned int indexCount      = 0;

    float animationSpeed         = 0.1f;
    float2 uvOffset              = float2::zero;
    float2 uvOffsetDirection     = float2::one;

    MeshComponent* meshComp = nullptr;
};
