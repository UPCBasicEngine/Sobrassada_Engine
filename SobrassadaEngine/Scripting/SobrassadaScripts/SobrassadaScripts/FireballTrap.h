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
    bool activated                      = false;
    float activationRange               = 10.0f;
    float attackCooldown                = 5.0f;
    int damage                          = 1;
    float damageDuration                = 1.5f;
    bool attacking                      = false;

    MeshComponent* trapMesh             = nullptr;
    SphereColliderComponent* damageArea = nullptr;

    float lastAttackTime                = -1.0f;
    float lastHitTime                   = -1.0f;
    bool damageActive                   = false;

    // fireball
    std::string fireballName            = "";
    GameObject* fireball                = nullptr;
    float rotationSpeed                 = 1.0f;
    float verticalSpeed                 = 0.0f;
    float gravity                       = -9.81f;
    float maxFallSpeed                  = -20.0f;
};
