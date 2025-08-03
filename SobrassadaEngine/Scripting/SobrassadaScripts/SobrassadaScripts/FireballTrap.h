#pragma once

#include <random>

#include "Script.h"
#include <array>

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

class FireballTrap : public Script
{
  public:
    FireballTrap(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    int GetDamage() const { return cfg.impactDamage; }
    void RecycleGO(GameObject* go) const;

    GameObject* SpawnIndicator(const float3& worldPos, float radius);

  private:
    void StartAttack();
    void HandleImpact();
    void DisableDamage();
    void UpdateFireball(float deltaTime);
    void UpdateMinis(float deltaTime);
    void SpawnMiniCluster();

    GameObject* RequestMini();
    GameObject* RequestImpactDecal();

    float GenerateRandomAttackTime(float min, float max) const;
    float3 RandomSpawnPoint() const;
    CameraMovement* FindShakeCamera() const;
    void SetupInspectorFields();

  private:
    struct TimedVFX
    {
        GameObject* go = nullptr;
        float delay    = 0.f;
        float life     = 2.f;
        float timer    = 0.f;
        bool active    = false;
    };

    static constexpr int EXTRA_VFX_COUNT = 11;
    std::array<TimedVFX, EXTRA_VFX_COUNT> extraVfx {};
    float vfxClock = 0.f;


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

        // Arc
        float maxLaunchRadius   = 6.f;
        float launchYawDeg      = 0.0f;
    };

    FireballTrapSettings cfg;

    // State Machine
    ACTIVATION_STATE activationState        = ACTIVATION_STATE::SLEEPING;
    float randomAttackTime                  = 0.f; // seconds until next shot
    float activatedTime                     = 0.f;
    float impactElapsed                     = 0.f; // burning timer after impact
    float dropElapsed                       = 0.f; // time since fireball spawned

    // Scene referennces
    MeshComponent* groundMesh               = nullptr;
    SphereColliderComponent* damageCollider = nullptr;
    GameObject* fireball                    = nullptr;
    GameObject* fireballShadow              = nullptr;
    CameraMovement* shakeCam                = nullptr;
    CubeColliderComponent* spawnZone        = nullptr;

    // Minis and decals
    GameObject* miniPrototype               = nullptr;
    GameObject* impactPrefab                = nullptr;
    GameObject* currentDecal                = nullptr;
    uint32_t miniCount                      = 4; // how many minis per big impact
    float miniLifeTime                      = 2.f;

    // Mini params
    struct MiniInstance
    {
        GameObject* go;
        float3 vel;
        float life;
    };
    std::vector<MiniInstance> activeMinis;

    struct MiniDecal
    {
        GameObject* go;
        float timer;
    };
    std::vector<MiniDecal> activeMiniDecals;

    // Indicator (where will fall)
    GameObject* indicatorPrefab = nullptr;
    GameObject* activeIndicator = nullptr;
    float indicatorPulse        = 0.0f;
    float indicatorScale        = 1.0f;
    float3 indicatorBaseScale   = float3::one;

    float3 impactOffsetLocal    = float3::zero; // XY of impact relative to base
    float3 fireVelocity         = float3::zero;
    float3 shadowBaseScale      = float3::one;
    float3 lastImpactWorld      = float3::zero;
    bool allowMiniDecals        = true;
    float noMiniHitRadius       = 1.0f;

    mutable std::mt19937 rng {std::random_device {}()};
};
