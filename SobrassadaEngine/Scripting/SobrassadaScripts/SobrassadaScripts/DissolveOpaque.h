#pragma once

#include "Script.h"

class ResourceTexture;
class MeshComponent;

class DissolveOpaque : public Script
{
  public:
    DissolveOpaque(GameObject* parent);
    ~DissolveOpaque() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;

    void Reset() override;

  private:
    unsigned int shaderProgram    = 0;

    unsigned int vao              = 0;
    unsigned int vbo              = 0;
    unsigned int ebo              = 0;

    float timer                   = 0.f;
    float dissolveDuration        = 1.f;

    UID noiseTextureUID           = 0;
    bool isFinished               = false;
    bool properlyInitialized      = false;

    ResourceTexture* noiseTexture = nullptr;
    MeshComponent* meshComp       = nullptr;
    unsigned int indexCount       = 0;

    bool isAlphaDiscard           = false;
};
