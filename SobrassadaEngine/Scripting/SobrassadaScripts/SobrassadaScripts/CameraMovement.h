#pragma once

#include "Script.h"

class GameObject;
class CharacterControllerComponent;

class CameraMovement : public Script
{
  public:
    CameraMovement(GameObject* parent);
    virtual ~CameraMovement() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void EnableMouseOffset(bool enable) { mouseOffsetEnabled = enable; }
    void SetPosition(const float3& newPos);

  private:
    void FollowTarget(float deltaTime);

  private:
    std::string targetName;
    const GameObject* target                       = nullptr;
    const CharacterControllerComponent* controller = nullptr;

    float3 finalPosition                           = float3::zero;
    float smoothnessVelocity                       = 10.0f;

    bool mouseOffsetEnabled                        = false;
    float mouseOffsetIntensity                     = 0.0f;

    bool lookAheadEnabled                          = false;
    float lookAheadIntensity                       = 0.0f;
    float currentLookAhead                         = 0.0f;
    float lookAheadSmoothness                      = 0.0f;

    float followDistanceThreshold                  = 0.0f;
    bool isFollowing                               = false;
};