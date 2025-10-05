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

    bool isAlphaDiscard         = false;

    unsigned int indexCount     = 0;

    float frameTimer            = 0.01f;
    float animationFPS          = 0.0f;
    bool isAdditive             = false;

    MeshComponent* meshComp     = nullptr;
    std::string fragment        = "";
    std::string vertex          = "";

    float3 color1               = float3(0.188f, 0.357f, 0.733f);
    float3 color2               = float3(0.153f, 0.941f, 0.957f);
    float3 color3               = float3(1.0f, 1.0f, 1.0f);
    float3 color4               = float3(1.0f, 1.0f, 1.0f);
};
