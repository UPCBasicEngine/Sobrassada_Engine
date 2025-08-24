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

    void SetLife(float newLife)
    {
        prevLife  = nextLife;
        nextLife  = newLife;
        startTime = time;
    }
    void OnHit() { hitTimer = 0.3f; }

  private:
    unsigned int shaderProgram = 0;

    unsigned int vao           = 0;
    unsigned int vbo           = 0;
    unsigned int texture       = 0;

    float time                 = 0.0f;
    float noiseTiling          = 5.0f;
    float noiseSpeed           = 0.05f;

    float nextLife             = 3.0f;
    float prevLife             = 3.0f;
    float startTime            = 0.0f;
    float hitTimer             = 0.0f;

    ImageComponent* imageComp  = nullptr;
};
#pragma once
