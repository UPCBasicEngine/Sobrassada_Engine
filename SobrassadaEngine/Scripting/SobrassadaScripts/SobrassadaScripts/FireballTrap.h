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
    float fallingHeight     = 20.0f; // spawn Y offset
    float gravity           = 9.81f; // drop acceleration
    float maxFallSpeed      = 20.0f;
    float rotationSpeed     = 90.0f; // deg/s spin while falling
    int impactDamage        = 1;
    float bigBurnRadius     = 2.0f;
    float bigBurnDuration   = 3.0f;

    // Mini fireball (division)
    int splitChildren       = 3; // 0‑3 children per split
    float splitSpreadDeg    = 40.f;
    int splitDepth          = 1;
    float miniScale         = 0.6f; // size of minis vs big ball
    float miniBurnRadius    = 1.0f;
    float miniBurnDuration  = 2.0f;

    // Arc
    float maxLaunchRadius   = 6.f;
    float launchYawDeg      = 0.0f;
};

class FireballTrap : public Script
{
  public:
    FireballTrap(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    int GetDamage() const { return cfg.impactDamage; }
    void RecycleGO(GameObject* go);

    GameObject* SpawnIndicator(const float3& worldPos, float radius);

  private:
    // StateMachine helpers
    void StartAttack();
    void HandleImpact();
    void DisableDamage();
    void UpdateFireball(float deltaTime);
    void UpdateMinis(float deltaTime);
    void SpawnMiniCluster();

    // Mini
    GameObject* RequestMini();
    GameObject* RequestImpactDecal();

    float GenerateRandomAttackTime(float min, float max) const;
    float3 RandomSpawnPoint() const;
    CameraMovement* FindShakeCamera();
    void SetupInspectorFields();

  private:
    float3 spawnCenter                      = float3::zero; // local offset of epicenter
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

    GameObject* impactPrefab  = nullptr;
    GameObject* currentDecal  = nullptr;

    // Mini params
    uint32_t miniCount        = 4; // how many minis per big impact
    float miniSpeed           = 5.f;
    float miniLifeTime        = 2.f;
    struct MiniInstance
    {
        GameObject* go;
        float3 vel;
        float life;
    };
    std::vector<MiniInstance> activeMinis;

    float4x4 baseLocal;                      // original local transform
    float3 impactOffsetLocal = float3::zero; // XY of impact relative to base

    float3 fireVelocity      = float3::zero;
    float3 shadowBaseScale   = float3::one;

    struct MiniDecal
    {
        GameObject* go;
        float timer;
    };
    std::vector<MiniDecal> activeMiniDecals;

    GameObject* indicatorPrefab = nullptr;
    GameObject* activeIndicator = nullptr;
    float indicatorPulse        = 0.0f;
    float indicatorScale        = 1.0f;
    float3 indicatorBaseScale   = float3::one;

    float3 lastImpactWorld      = float3::zero;
    bool allowMiniDecals        = true; 
    float noMiniHitRadius       = 1.0f;

};
