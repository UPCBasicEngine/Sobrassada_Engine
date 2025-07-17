#pragma once

#include "Script.h"

#include "Math/float2.h"

class MovingUVLight : public Script
{
  public:
    MovingUVLight(GameObject* parent) : Script(parent) {};
    ~MovingUVLight() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

  private:
    unsigned int shaderProgram   = 0;

    unsigned int vao             = 0;
    unsigned int vbo             = 0;
    unsigned int ebo             = 0;

    unsigned int diffuseTexture  = 0;
    unsigned int metallicTexture = 0;
    unsigned int specularTexture = 0;
    unsigned int normalTexture   = 0;

    bool matIsMetallic           = false;
    bool isAlphaDiscard          = false;
    float roughnessFactor        = 1.f;
    float metallicFactor         = 1.f;

    unsigned int indexCount      = 0;

    float animationSpeed         = 0.1f;
    float2 uvOffset              = float2::zero;
};
