#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;

class MirageVFX : public Script
{
  public:
    MirageVFX(GameObject* parent, const std::string& ver, const std::string& frag);
    ~MirageVFX() override;

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

    float frameTimer            = 0.01f;
    float animationFPS          = 0.0f;
    float sharpness             = 2.5f;
    bool isAdditive             = false;

    MeshComponent* meshComp     = nullptr;
    std::string fragment        = "";
    std::string vertex          = "";
};
