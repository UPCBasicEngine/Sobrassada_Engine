#pragma once

#include "HashString.h"
#include "Math/float3.h"
#include "Script.h"

class CapsuleColliderComponent;

class Projectile : public Script
{
  public:
    Projectile(GameObject* parent);
    virtual ~Projectile() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void Shoot(const float3& origin, const float3& direction);
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;
    void OnWallHit();

    int GetDamage() const { return damage; }
    GameObject* GetParent() const { return parent; }
    void Hit(GameObject* otherObject);

  private:
    void Move(float deltaTime);
    void StopProjectile();

  private:
    CapsuleColliderComponent* collider = nullptr;

    float3 direction                   = float3::zero;
    float speed                        = 10.0f;

    float range                        = 10.0f;
    float3 startPos                    = float3::zero;

    int damage                         = 1;
    float frames                       = 0.0f;

    bool hasHitTarget                  = false;

    bool isStuckInWall                 = false;
    float stuckTimer                   = 0.0f;
    float stuckDuration                = 0.35f;
    HashString wallTag;

    bool isActive = false;
};