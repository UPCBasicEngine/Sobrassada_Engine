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

  private:
    unsigned int shaderProgram       = 0;

    unsigned int vao                 = 0;
    unsigned int vbo                 = 0;
    unsigned int ebo                 = 0;

    unsigned int diffuseTex          = 0;
    unsigned int specularMetallicTex = 0;
    unsigned int normalTex           = 0;
    unsigned int emissiveTex         = 0;

    bool isAlphaDiscard              = false;
    bool matIsMetallic               = false;

    float roughnessFactor            = 1.f;
    float metallicFactor             = 1.f;

    unsigned int indexCount          = 0;

    float animationSpeed             = 0.1f;
    float2 uvOffset                  = float2::zero;
    float2 uvOffsetStart             = float2::zero;
    float2 uvOffsetDirection         = float2::one;

    MeshComponent* meshComp          = nullptr;
};
