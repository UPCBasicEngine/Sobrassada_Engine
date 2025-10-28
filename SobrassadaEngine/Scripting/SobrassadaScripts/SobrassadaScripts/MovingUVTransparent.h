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

    void Reset() override;

    bool IsPaused() const { return isPaused; }
    void SetPaused(bool newValue) { isPaused = newValue; }

  private:
    unsigned int shaderProgram  = 0;

    unsigned int vao            = 0;
    unsigned int vbo            = 0;
    unsigned int ebo            = 0;

    bool isAlphaDiscard         = false;
    bool isDoubleSided          = false;
    bool isPaused               = false;

    unsigned int indexCount     = 0;

    float animationSpeed        = 0.1f;
    float2 uvOffset             = float2::zero;
    float2 uvOffsetDirection    = float2::one;
    float2 uvOffsetStart        = float2::zero;

    MeshComponent* meshComp     = nullptr;

    bool isFadeOut              = false;
    float fadeOutTime           = 0.0f;
    float fadeOutTimer          = 0.0f;
    float fadeOutDuration       = 0.0f;
};
