#pragma once

#include "Script.h"

#include "Math/float2.h"

class ImageComponent;

class BarFill : public Script
{
  public:
    BarFill(GameObject* parent);
    ~BarFill() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;
    void Reset() override;

    void SetFillAmount(float newFill);

  private:
    unsigned int shaderProgram = 0;

    unsigned int vao           = 0;
    unsigned int vbo           = 0;
    unsigned int texture       = 0;

    float nextFillAmount       = 0.0f;
    float prevFillAmount       = 0.0f;
    float transitionTime       = 0.0f;
    float time                 = 0.0f;
    float startTime            = 0.0f;

    float waveAmplitude        = 0.0f;
    float waveFrequency        = 0.0f;
    float waveSpeed            = 0.0f;

    float textureStart         = 0;
    float textureEnd           = 0;
    float textureWidth         = 0;

    ImageComponent* imageComp  = nullptr;
};
