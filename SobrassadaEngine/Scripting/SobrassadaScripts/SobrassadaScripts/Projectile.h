#pragma once

#include "Script.h"

#include "Math/float3.h"

class CapsuleColliderComponent;
class MeshComponent;
class AttackVfxSpritesheet;

class Projectile : public Script
{
  public:
    Projectile(GameObject* parent);
    virtual ~Projectile() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void Shoot(const float3& origin, const float3& direction);
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

    int GetDamage() const { return damage; }
    GameObject* GetParent() const { return parent; }
    void Hit(GameObject* otherObject);

  private:
    void Move(float deltaTime);

  private:
    CapsuleColliderComponent* collider = nullptr;

    std::string spritesheetNameV       = "RangedVFX_v";
    std::string spritesheetNameH       = "RangedVFX_h";
    MeshComponent* spritesheetMeshV    = nullptr;
    MeshComponent* spritesheetMeshH    = nullptr;
    AttackVfxSpritesheet* spritesheetV = nullptr;
    AttackVfxSpritesheet* spritesheetH = nullptr;

    float3 direction                   = float3::zero;
    float speed                        = 10.0f;

    float range                        = 10.0f;
    float3 startPos                    = float3::zero;

    int damage                         = 1;
    float frames                       = 0.0f;

    bool hasHitTarget                  = false;

    std::string particlesName          = "Spear_PS";
    GameObject* particles              = nullptr;
};