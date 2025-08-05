#pragma once

#include "Script.h"

class GameObject;
class MeshComponent;
class SphereColliderComponent;

class FireballTrap : public Script
{
    enum ACTIVATION_STATE
    {
        SLEEPING,
        IDLE,
        DROPPING,
        DAMAGING,
    };

  public:
    FireballTrap(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

    int GetDamage() const { return damage; }

  private:
    void StartAttack();
    void HandleImpact();
    void DisableDamage();
    void UpdateFireball(float deltaTime);
    float GenerateRandomAttackTime(float min, float max);

  private:
    float activationRange                   = 10.0f;
    float minAttackCooldown                 = 0.5f;
    float maxAttackCooldown                 = 3.0f;
    float randomAttackTime                  = 0.0f;
    int damage                              = 1;
    float damageDuration                    = 1.5f;

    MeshComponent* groundMesh               = nullptr;
    SphereColliderComponent* damageCollider = nullptr;

    float activatedTime                     = 0.0f;

    // fireball
    GameObject* fireball                    = nullptr;
    // fireball
    GameObject* fireballShadow              = nullptr;

    float verticalSpeed                     = 0.0f;
    float rotationSpeed                     = 1.0f;
    float fallingHeight                     = 20.0f;
    float editableMaxFallSpeed              = 20.0f;
    float editableGravity                   = 9.81f;

    ACTIVATION_STATE activationState        = SLEEPING;
};
