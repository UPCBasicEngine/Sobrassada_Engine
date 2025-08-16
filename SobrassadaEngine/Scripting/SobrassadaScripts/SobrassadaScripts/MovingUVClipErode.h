#pragma once
#include "Math/float2.h"
#include "Script.h"

class MeshComponent;
class CameraComponent;

class MovingUVClipErode : public Script
{
  public:
    MovingUVClipErode(GameObject* parent);
    ~MovingUVClipErode();

    bool Init() override;
    void Update(float dt) override;
    void Render(float dt, CameraComponent* cameraComp) override;
    void Reset() override;

  private:
    MeshComponent* meshComp    = nullptr;

    unsigned int shaderProgram = 0;
    unsigned int vao = 0, vbo = 0, ebo = 0, materialBuffer = 0;
    unsigned int indexCount  = 0;

    float animationSpeed     = 0.f;
    float2 uvOffsetDirection = float2(0, 0);
    bool isDoubleSided       = false;
    float2 uvOffsetStart     = float2(0, 0);
    float2 uvOffset          = float2(0, 0);
    bool isAlphaDiscard      = false;

    float erosionLevel       = 0.0f;  // [-1..1] Alpha = TexA - erosionLevel
    float edgeFeather        = 0.05f; // fade side to clip
    bool clipTo01            = true; 
    float2 clipMin           = float2(0, 0);
    float2 clipMax           = float2(1, 1);
};
