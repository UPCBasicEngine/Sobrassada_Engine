#pragma once

#include "Script.h"

class GameObject;
class MeshComponent;
class SphereColliderComponent;

class FireballTrap : public Script
{
  public:
    FireballTrap(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

    int GetDamage() const { return damage; }

  private:
    void StartAttack(float gameTime);
    void HandleImpact(float gameTime);
    void DisableDamage();
    void UpdateFireball(float deltaTime);
    float GenerateRandomAttackTime(float min, float max);

  private:
    bool activated                          = false;
    float activationRange                   = 10.0f;
    float minAttackCooldown                 = 0.5f;
    float maxAttackCooldown                 = 3.0f;
    float randomAttackTime                  = -1.0f;
    int damage                              = 1;
    float damageDuration                    = 1.5f;
    bool attacking                          = false;

    MeshComponent* groundMesh               = nullptr;
    SphereColliderComponent* damageCollider = nullptr;

    float lastAttackTime                    = -1.0f;
    float lastHitTime                       = -1.0f;
    bool isDealingDamage                    = false;

    // fireball
    GameObject* fireball                    = nullptr;
    // fireball
    GameObject* fireballShadow              = nullptr;
    
    float verticalSpeed                     = 0.0f;
    bool hasImpacted                        = false;
    float rotationSpeed                     = 1.0f;
    float fallingHeight                     = 20.0f;
    float maxFallSpeed                      = -20.0f;
    float editableMaxFallSpeed              = 20.0f;
    float gravity                           = -9.81f;
    float editableGravity                   = 9.81f;
};
