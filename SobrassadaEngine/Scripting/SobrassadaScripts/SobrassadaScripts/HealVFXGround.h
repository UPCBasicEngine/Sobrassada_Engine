#pragma once
#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;

class HealVFXGround : public Script
{
  public:
    HealVFXGround(GameObject* parent, const std::string& ver, const std::string& frag);
    ~HealVFXGround() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

    void Reset() override;

  private:
    unsigned int shaderProgram  = 0;

    unsigned int vao            = 0;
    unsigned int vbo            = 0;
    unsigned int ebo            = 0;
    unsigned int materialBuffer = 0;

    bool isAlphaDiscard         = false;

    unsigned int indexCount     = 0;

    float frameTimer            = 0.1f;
    float animationFPS          = 0.0f;
    bool isAdditive             = false;

    MeshComponent* meshComp     = nullptr;
    std::string fragment        = "";
    std::string vertex          = "";
};
