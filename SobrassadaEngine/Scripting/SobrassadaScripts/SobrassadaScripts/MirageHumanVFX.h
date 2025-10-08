#pragma once

#include "Script.h"

#include "Math/float2.h"

class MeshComponent;

class MirageHumanVFX : public Script
{
  public:
    MirageHumanVFX(GameObject* parent, const std::string& ver, const std::string& frag);
    ~MirageHumanVFX() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

  private:
    unsigned int shaderProgram  = 0;

    unsigned int vao            = 0;
    unsigned int vbo            = 0;
    unsigned int ebo            = 0;

    bool isAlphaDiscard         = false;

    unsigned int indexCount     = 0;

    bool isAdditive             = false;

    float3 colorTint            = float3(1.0f, 1.0f, 1.0f);

    MeshComponent* meshComp     = nullptr;
    std::string fragment        = "";
    std::string vertex          = "";
};
