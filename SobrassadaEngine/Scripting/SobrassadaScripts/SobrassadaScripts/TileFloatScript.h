#pragma once
#include "Math/Quat.h"
#include "Script.h"

class AudioSourceComponent;

class TileFloatScript : public Script
{
  public:
    TileFloatScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    inline float QuaternionDot(const Quat& a, const Quat& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

  private:
    bool isSetupCorrectly  = true;
    
    float speed = 10.0f;
    Quat finalRotation, startQuat, currentRotationQuat;
    float3 finalPosition, finalScale, startPosition, startRotation;
    float3 startScale               = float3(1.0f, 1.0f, 1.0f);
    float minDistanceToPlayer       = 10.0f;
    bool isActive                   = false;
    bool isFinished                 = false;

    float risingCounter             = 0.f;

    GameObject* tileToMove          = nullptr;

    // Audio
    AudioSourceComponent* audioComp = nullptr;
};