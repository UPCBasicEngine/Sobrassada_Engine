#pragma once

#include "ResourceAnimation.h"
#include "Script.h"
#include "Standalone/AnimController.h"
#include "Standalone/AnimationComponent.h"
#include <array>
#include <vector>

class GameObject;
class MeshComponent;
class SphereColliderComponent;
class CameraMovement;
class CameraComponent;
class CubeColliderComponent;
class ShaderScriptComponent;

enum class ACTIVATION_STATE
{
    SLEEPING,
    IDLE,
    DROPPING,
    DAMAGING,
};

class FireballTrap : public Script
{
  public:
    FireballTrap(GameObject* parent);
    ~FireballTrap();
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
    void OnBigBallHitPlayer() { bigBallHitPlayerThisAttack = true; }

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
        float activationRange   = 10.f;
        float minAttackCooldown = 0.5f;
        float maxAttackCooldown = 3.f;

        float fallingHeight     = 20.0f;
        float gravity           = 9.81f;
        float maxFallSpeed      = 20.0f;
        float rotationSpeed     = 90.0f;
        int impactDamage        = 1;
        float bigBurnRadius     = 2.0f;
        float bigBurnDuration   = 3.0f;

        float maxLaunchRadius   = 6.f;
        float launchYawDeg      = 0.0f;

        float miniStartHeight   = 0.5f;
        float miniUpSpeed       = 4.5f;
        float miniLandingRadius = 0.4f;
    };

    FireballTrapSettings cfg;

    // State Machine
    ACTIVATION_STATE activationState            = ACTIVATION_STATE::SLEEPING;
    float randomAttackTime                      = 0.f;
    float activatedTime                         = 0.f;
    float impactElapsed                         = 0.f;
    float dropElapsed                           = 0.f;

    // Scene references
    MeshComponent* groundMesh                   = nullptr;
    SphereColliderComponent* damageAreaCollider = nullptr;
    GameObject* fireball                        = nullptr;
    GameObject* fireballShadow                  = nullptr;
    CameraMovement* shakeCam                    = nullptr;
    CubeColliderComponent* spawnZone            = nullptr;

    // Minis and decals
    GameObject* miniPrototype                   = nullptr;
    GameObject* impactPrefab                    = nullptr;
    GameObject* currentDecal                    = nullptr;
    uint32_t miniCount                          = 4;
    float miniLifeTime                          = 2.f;

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
        GameObject* vfx   = nullptr;
        float delay       = 0.f;
        float life        = 1.f;
        float3 localPos   = float3::zero;
        float3 localScale = float3::one;

        bool triggered    = false;
        float timer       = 0.f;

        std::vector<ShaderScriptComponent*> shaders;
    };

    std::vector<VFXEvent> scheduledVfx;
    float vfxSchedClock = 0.f;

    void ScheduleVfx(GameObject* vfx, float delay, float life, const float3& pos, const float3& scale = float3::one);
    void UpdateScheduledVfx(float dt);
    void ClearScheduledVfx();
    void EnableVFX(GameObject* vfx, bool enable);
    void ResetVFX(GameObject* vfx);

    // VFX GameObjects (no pooling)
    GameObject* vfxMainLight   = nullptr;
    GameObject* vfxLightImpact = nullptr;
    GameObject* vfxFireImpact  = nullptr;
    GameObject* vfxBombGround  = nullptr; 
    GameObject* vfxBlackStain  = nullptr;
    GameObject* vfxIndicator   = nullptr;
    GameObject* vfxBombIndicatorSmallSymbol = nullptr;
    std::vector<GameObject*> miniIndicatorVfx;


    // Store mesh components for each VFX
    std::vector<MeshComponent*> vfxIndicatorMeshes;
    std::vector<MeshComponent*> vfxMainLightMeshes;
    std::vector<MeshComponent*> vfxLightImpactMeshes;
    std::vector<MeshComponent*> vfxFireImpactMeshes;
    std::vector<MeshComponent*> vfxBombGroundMeshes;
    std::vector<MeshComponent*> vfxBlackStainMeshes;
    std::vector<MeshComponent*> vfxBombIndicatorSmallSymbolMeshes;

    // Prefabs (for reference)
    GameObject* vfxMainLightPrefab     = nullptr;
    GameObject* vfxLightImpactPrefab   = nullptr;
    GameObject* vfxFireImpactPrefab    = nullptr;
    GameObject* vfxBombGroundPrefab    = nullptr;
    GameObject* vfxBlackStainPrefab    = nullptr;
    GameObject* vfxIndicatorPrefab     = nullptr;
    GameObject* miniIndicatorVfxPrefab = nullptr;
    GameObject* vfxBombIndicatorSmallSymbolPrefab = nullptr;

    // VFX Delays
    float vfxMainLightDelay            = 0.00f;
    float vfxLightImpactDelay          = 0.00f;
    float vfxFireImpactDelay           = 0.15f;
    float vfxBombGroundDelay           = 0.35f;
    float vfxBlackStainDelay           = 0.70f;

    // VFX Lifetimes
    float vfxMainLightLife             = 0.6f;
    float vfxLightImpactLife           = 0.4f;
    float vfxFireImpactLife            = 1.5f;
    float vfxBombGroundLife            = 3.0f;
    float vfxBlackStainLife            = 2.5f;

    float3 impactLocalPos              = float3::zero;
    float3 fireballVelocity            = float3::zero;
    float3 shadowBaseScale             = float3::one;
    float3 lastImpactWorld             = float3::zero;
    float vfxIndicatorWorldRadius      = 0.6f;

    float miniIndicatorVfxScale        = 0.4f;
    std::vector<float> plannedMiniAngles;

    bool bigBallHitPlayerThisAttack = false;

    // Animations
    GameObject* animSPrefab         = nullptr;
    GameObject* animNPrefab         = nullptr;
    GameObject* animWPrefab         = nullptr;

    std::string animSName           = "Bomb_animation_S";
    std::string animNName           = "Bomb_animation_N";
    std::string animWName           = "Bomb_animation_W";

    void PlayBombAnimationsAt(const float3& localPos);
    void StopBombAnimations();

    struct OneShotAnim
    {
        GameObject* root       = nullptr;
        AnimationComponent* ac = nullptr;
        bool playing           = false;
    };

    OneShotAnim animS;
    OneShotAnim animN;
    OneShotAnim animW;

    void PlayAnimationAt(OneShotAnim& anim, const float3& localPos);
    void UpdateAnimation(OneShotAnim& anim, float deltaTime);
    void StopAnimation(OneShotAnim& anim);
    bool InitAnimation(OneShotAnim& anim, GameObject* prefab, const std::string& name);

    struct MiniImpactAnim
    {
        GameObject* root       = nullptr;
        AnimationComponent* ac = nullptr;
        bool playing           = false;
        float timer            = 0.f;
        float lifetime         = 5.0f;
    };

    void PlayMiniImpactAnimation(const float3& localPos);

    void OnPlayerExitLocation();

    GameObject* CloneHierarchy(GameObject* src, UID newParentUID);
    void StopAnimationsRecursive(GameObject* go);

    std::vector<ShaderScriptComponent*> indicatorShaders;
    std::vector<MeshComponent*> indicatorMeshes;

    static constexpr int MINI_SLOTS = 4;
    OneShotAnim miniS[MINI_SLOTS];
    std::array<std::string, MINI_SLOTS> miniSNames {
        "Bomb_animation_S_1", "Bomb_animation_S_2", "Bomb_animation_S_3", "Bomb_animation_S_4"
    };
    int miniSNext = 0;
};