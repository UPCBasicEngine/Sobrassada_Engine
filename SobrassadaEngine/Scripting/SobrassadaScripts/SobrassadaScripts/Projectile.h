#pragma once

#include "Script.h"

#include "Math/float3.h"

class CubeColliderComponent;

class Projectile : public Script
{
  public:
    Projectile(GameObject* parent);
    virtual ~Projectile() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void Shoot(const float3& origin, const float3& direction);
    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

    int GetDamage() const { return damage; }

  private:
    void Move(float deltaTime);

  private:
    CubeColliderComponent* collider = nullptr;

    float3 direction                = float3::zero;
    float speed                     = 10.0f;

    float range                     = 10.0f;
    float3 startPos                 = float3::zero;

    int damage                      = 1;
    float frames                    = 0.0f;
};