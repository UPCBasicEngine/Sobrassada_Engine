#pragma once

#include <random>

#include "Script.h"

class GameObject;
class MeshComponent;
class SphereColliderComponent;
class CameraMovement;
class CameraComponent;
class CubeColliderComponent;

// trap lifecycle (big ball -> impact -> cooldown)
enum class ACTIVATION_STATE
{
    SLEEPING, // waiting, player out of range
    IDLE,     // armed, counting down to next attack
    DROPPING, // big fireball falling
    DAMAGING, // ground is burning, damage collider active
};

struct FireballTrapSettings
{
    // Activation radius & cooldowns
    float activationRange   = 10.f; 
    float minAttackCooldown = 0.5f; 
    float maxAttackCooldown = 3.f;  

    // Big fireball physics
    float fallingHeight     = 20.f;  // spawn Y offset
    float gravity           = 9.81f; // drop acceleration
    float maxFallSpeed      = 20.f;
    float rotationSpeed     = 90.f;  // deg/s spin while falling
    int impactDamage        = 1;
    float bigBurnRadius     = 2.f;  
    float bigBurnDuration   = 3.f;

    // Mini fireball (division)
    int splitChildren       = 3;    // 0‑3 children per split
    float splitSpreadDeg    = 40.f; 
    int splitDepth          = 1;    
    float miniScale         = 0.6f; // size of minis vs big ball
    float miniBurnRadius    = 1.f;  
    float miniBurnDuration  = 2.f;
};

class FireballTrap : public Script
{
  public:
    FireballTrap(GameObject* parent);                  
    bool Init() override;                              
    void Update(float deltaTime) override;             
    int GetDamage() const { return cfg.impactDamage; } 

  private:
    // StateMachine helpers
    void StartAttack();            
    void HandleImpact();           
    void DisableDamage();          
    void UpdateFireball(float deltaTime); 
    void UpdateMinis(float deltaTime); 
    void SpawnMiniCluster();  

    // Pool helpers
    GameObject* RequestMini();        // returns an enabled mini from pool
    void RecycleMini(GameObject*);    // disable & recycle mini
    GameObject* RequestImpactDecal(); // decal pool
    void RecycleImpactDecal(GameObject*);

    float GenerateRandomAttackTime(float min, float max) const;
    float3 RandomSpawnPoint() const;                           
    CameraMovement* FindShakeCamera();                         
    void SetupInspectorFields();                               

  private:
    float3 spawnCenter                      = float3::zero;          // local offset of epicenter
    float3 spawnHalfSize                    = float3(5.f, 0.f, 5.f);

    float randomAttackTime                  = 0.f; // seconds until next shot
    float activatedTime                     = 0.f;
    float impactElapsed                     = 0.f; // burning timer after impact
    float dropElapsed                       = 0.f; // time since fireball spawned

    MeshComponent* groundMesh               = nullptr;
    SphereColliderComponent* damageCollider = nullptr;
    GameObject* fireball                    = nullptr;
    GameObject* fireballShadow              = nullptr;
    CameraMovement* shakeCam                = nullptr;
    CubeColliderComponent* spawnZone        = nullptr;

    ACTIVATION_STATE activationState        = ACTIVATION_STATE::SLEEPING;

    FireballTrapSettings cfg;                           
    mutable std::mt19937 rng {std::random_device {}()}; 

    GameObject* miniPrototype = nullptr;
    std::vector<GameObject*> miniPool;
    uint32_t poolSize                      = 6;
    static constexpr uint32_t kMaxMiniPool = 50;

    GameObject* impactPrefab               = nullptr; // decal prefab
    std::vector<GameObject*> decalPool;
    uint32_t decalPoolSize                  = 3;
    static constexpr uint32_t kMaxDecalPool = 10;
    GameObject* currentDecal                = nullptr;

    // Mini params
    uint32_t miniCount                      = 4; // how many minis per big impact
    float miniSpeed                         = 5.f;
    float miniLifeTime                      = 2.f;
    struct MiniInstance
    {
        GameObject* go;
        float3 vel;
        float life;
    };
    std::vector<MiniInstance> activeMinis;

    float4x4 baseLocal;                      // original local transform
    float3 impactOffsetLocal = float3::zero; // XY of impact relative to base
};
