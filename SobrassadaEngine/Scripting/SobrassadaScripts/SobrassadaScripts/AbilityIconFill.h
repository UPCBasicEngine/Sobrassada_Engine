#pragma once

#include "Script.h"

#include "Math/float2.h"

class ImageComponent;
class ResourceTexture;

class AbilityIconFill : public Script
{
  public:
    AbilityIconFill(GameObject* parent);
    ~AbilityIconFill() override;

    bool Init() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime, CameraComponent* cameraComp) override;
    void Reset() override;

    void SetFillAmount(float newFill);

  private:
    unsigned int shaderProgram  = 0;

    unsigned int vao            = 0;
    unsigned int vbo            = 0;
    unsigned int texture        = 0;

    float fillAmount            = 0.0f;

    float waveAmplitude         = 0.0f;
    float waveFrequency         = 0.0f;
    float waveSpeed             = 0.0f;

    ImageComponent* imageComp   = nullptr;
    ResourceTexture* otherImage = nullptr;
    UID otherImageUID           = 0;
    UID otherImageBindlessUID   = 0;
};
