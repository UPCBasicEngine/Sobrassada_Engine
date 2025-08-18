#pragma once

#include "Script.h"

class ImageComponent;

class DamageMask : public Script
{
  public:
    DamageMask(GameObject* parent);
    ~DamageMask() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;
    void Reset() override;

  private:
    unsigned int shaderProgram = 0;

    unsigned int vao           = 0;
    unsigned int vbo           = 0;
    unsigned int texture       = 0;

    float time                 = 0.0f;
    float pulseSpeed           = 0.0f;
    float intensity            = 0.0f;
    float noiseTiling          = 5.0f;
    float noiseSpeed           = 0.05f;

    ImageComponent* imageComp  = nullptr;
};
#pragma once
