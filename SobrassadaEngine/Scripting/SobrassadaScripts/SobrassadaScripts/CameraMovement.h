#pragma once

#include "Script.h"
#include <random>

class GameObject;
class CharacterControllerComponent;

class CameraMovement : public Script
{
  public:
    CameraMovement(GameObject* parent);
    virtual ~CameraMovement() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void EnableAimOffset(bool enable) { aimOffsetEnabled = enable; }
    void SetPosition(const float3& newPos);
    void StartShake(float duration, float intensity);

  private:
    void FollowTarget(float deltaTime);
    void CameraShake(float deltaTime);

  private:
    std::string targetName;
    const GameObject* target                       = nullptr;
    const CharacterControllerComponent* controller = nullptr;

    float3 finalPosition                           = float3::zero;
    float smoothnessVelocity                       = 10.0f;

    bool aimOffsetEnabled                          = false;
    float aimOffsetIntensity                       = 0.0f;

    bool lookAheadEnabled                          = false;
    float lookAheadIntensity                       = 0.0f;
    float currentLookAhead                         = 0.0f;
    float lookAheadSmoothness                      = 0.0f;

    float followDistanceThreshold                  = 0.0f;
    bool isFollowing                               = false;

    GameObject* camera                             = nullptr;
    float3 currentOffset                           = float3::zero;
    float shakeDuration                            = 0.0f;
    float shakeTimer                               = 0.0f;
    float shakeIntensity                           = 0.0f;
    float3 defaultCameraPos                        = float3::zero;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
};