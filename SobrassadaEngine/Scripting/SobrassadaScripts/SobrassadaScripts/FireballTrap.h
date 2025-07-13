#pragma once

#include "Script.h"

class GameObject;
class MeshComponent;
class SphereColliderComponent;

enum ACTIVATION_STATE
{
    SLEEPING,
    IDLE,
    DROPPING,
    DAMAGING,
};


struct FireballTrapSettings
{
    // -- ACTIVATION --
    float activationRange   = 10.f;
    float minAttackCooldown = 0.5f;
    float maxAttackCooldown = 3.f;

    // -- Big fireball --
    float fallingHeight     = 20.f;
    float gravity           = 9.81f;
    float maxFallSpeed      = 20.f;
    float rotationSpeed     = 90.f; // degree
    int impactDamage        = 1;
    float bigBurnRadius     = 2.f;
    float bigBurnDuration   = 3.f;

    // -- Subdivision --
    int splitChildren       = 3;    // 0-3 max division
    float splitSpreadDeg    = 40.f; // ?Total arc
    int splitDepth          = 1;    // Max 1 recursive
    float miniScale         = 0.6f;
    float miniBurnRadius    = 1.f;
    float miniBurnDuration  = 2.f;
    int miniImpactDamage    = 0; // Damage over time
};

class FireballTrap : public Script
{
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
    
    GameObject* RequestMini();
    void RecycleMini(GameObject* mini);
    void SpawnMiniCluster();
    void UpdateMinis(float deltaTime);  


  private:
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

    ACTIVATION_STATE activationState = SLEEPING;
    FireballTrapSettings cfg;
    GameObject* miniPrototype = nullptr;

    std::vector<GameObject*> miniPool; // container
    uint32_t poolSize = 6;             // default number clones

    // --- Split params ---
    uint32_t miniCount = 4;
    float miniSpeed    = 5.0f;
    float miniLifeTime = 2.0f; 

    struct MiniInstance
    {
        GameObject* go; // ref  clon
        float3 vel;    
        float life;    
    };

    std::vector<MiniInstance> activeMinis;
};
