#pragma once
#include "Script.h"

class MiniFireball : public Script
{
  public:
    MiniFireball(GameObject* parent) : Script(parent) {}
    int damage = 1;
    float life = 3.f;

    bool Init() override;
    void Update(float dt) override;
    void OnCollision(GameObject* other, const float3 normal, ColliderLayer layer) override;

    // Set initial random rotation from FireballTrap
    void SetInitialRotation(const float3& rotation) { initialRotation = rotation; }
    void SetRotationSpeed(const float3& speed) { rotationSpeed = speed; }

  private:
    float lifeTimer        = 0.f;
    GameObject* shadow     = nullptr;
    float3 baseScale       = float3::one;
    float3 rotationSpeed   = float3(90.f, 120.f, 60.f); // degrees per second
    float3 currentRotation = float3::zero;
    float3 initialRotation = float3::zero;
};