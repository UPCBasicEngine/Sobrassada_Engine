#pragma once

#include "Script.h"
#include <array>

class GameObject;
class MeshComponent;
class SphereColliderComponent;
class CameraMovement;
class CameraComponent;
class CubeColliderComponent;

// trap lifecycle big ball -> impact -> cooldown
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

        float miniStartHeight   = 0.5f; // spawn Y of each mini
        float miniUpSpeed       = 4.5f; // initial up velocity of each mini
        float miniLandingRadius = 0.4f; // where we expect minis to land (ring radius)
    };

    FireballTrapSettings cfg;

    // State Machine
    ACTIVATION_STATE activationState        = ACTIVATION_STATE::SLEEPING;
    float randomAttackTime                  = 0.f; // seconds until next shot
    float activatedTime                     = 0.f;
    float impactElapsed                     = 0.f; // burning timer after impact
    float dropElapsed                       = 0.f; // time since fireball spawned

    // Scene references
    MeshComponent* groundMesh               = nullptr;
    SphereColliderComponent* damageAreaCollider = nullptr;
    GameObject* fireball                    = nullptr;
    GameObject* fireballShadow              = nullptr;
    CameraMovement* shakeCam                = nullptr;
    CubeColliderComponent* spawnZone        = nullptr;

    // Minis and decals
    GameObject* miniPrototype               = nullptr;
    GameObject* impactPrefab                = nullptr;
    GameObject* currentDecal                = nullptr;
    uint32_t miniCount                      = 4; // how many minis
    float miniLifeTime                      = 2.f;

    // Mini params
    struct MiniInstance
    {
        GameObject* go;
        float3 velocity;
        float life;
    };
    std::vector<MiniInstance> activeMinis;

    // VFX scheduling
    struct VFXEvent
    {
        GameObject* prefab   = nullptr;      // source prefab to clone
        float delay          = 0.f;          // start time relative to StartAttack() (seconds)
        float life           = 1.f;          // auto-despawn after 'life' seconds
        float3 localPos      = float3::zero; // placement relative to trap base
        float3 localScale    = float3::one;

        bool triggered       = false;
        float timer          = 0.f;
        GameObject* instance = nullptr;
    };

    std::vector<VFXEvent> scheduledVfx;
    float vfxSchedClock = 0.f;

    // API
    void ScheduleVfx(GameObject* prefab, float delay, float life, const float3& pos, const float3& scale = float3::one);
    void UpdateScheduledVfx(float dt);
    void ClearScheduledVfx();

    // Prefabs (set via Inspector later)
    GameObject* vfxMainLightPrefab     = nullptr;
    GameObject* vfxLightImpactPrefab   = nullptr;
    GameObject* vfxFireImpactPrefab    = nullptr;
    GameObject* vfxBombGroundPrefab    = nullptr;
    GameObject* vfxBlackStainPrefab    = nullptr; 

    // Delays VFX
    float vfxMainLightDelay            = 0.00f; 
    float vfxLightImpactDelay          = 0.00f; 
    float vfxFireImpactDelay           = 0.15f;
    float vfxBombGroundDelay           = 0.35f;
    float vfxBlackStainDelay           = 0.70f;

    // Lifetimes VFX
    float vfxMainLightLife             = 0.6f;
    float vfxLightImpactLife           = 0.4f;
    float vfxFireImpactLife            = 1.5f;
    float vfxBombGroundLife            = 3.0f;
    float vfxBlackStainLife            = 2.5f;

    // Indicator (where will fall)
    GameObject* vfxIndicatorPrefab     = nullptr; // prefall indicator VFX

    float3 impactLocalPos           = float3::zero; // XY of impact relative to base
    float3 fireballVelocity                = float3::zero;
    float3 shadowBaseScale             = float3::one;
    float3 lastImpactWorld             = float3::zero;
    float vfxIndicatorWorldRadius      = 0.6f;

    // Mini impact VFX
    GameObject* miniIndicatorVfxPrefab = nullptr; // prefall indicator for minis
    float miniIndicatorVfxScale        = 0.4f;    // world radius/scale of the indicator
    std::vector<float> plannedMiniAngles;
};
