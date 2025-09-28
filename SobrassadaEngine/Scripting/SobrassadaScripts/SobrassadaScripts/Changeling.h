#pragma once

#include "Character.h"
#include "Standalone/Physics/CubeColliderComponent.h"

class SphereColliderComponent;
class AudioSourceComponent;
class GameObject;
class AIAgentComponent;
class Projectile;

enum class ChangelingVersions
{
    RANDOM,
    DEFAULT,
    BLOCK,
};

struct ChangelingDashTrailContainer
{
    GameObject* dashTrailObject               = nullptr;
    GameObject* dashTrailMidChildMeshObject   = nullptr;
    GameObject* dashTrailStartChildMeshObject = nullptr;
    GameObject* dashTrailEndChildMeshObject   = nullptr;
};

enum class ChangelingStates
{
    NONE                    = 0,
    IDLE_BURIED             = 1,
    PEEK                    = 2,
    DIG_UP_TRANSITION       = 3,
    DIG_DOWN_TRANSITION     = 4,
    IDLE_VISIBLE            = 5,
    CHASE                   = 6,
    BURIED_TRAVEL           = 7,
    DASH_ATTACK_PREPARATION = 8,
    DASH_ATTACK             = 9,
    DASH_ATTACK_WIGGLE      = 10,
    DASH_ATTACK_COOLDOWN    = 11,
    DASH_CHAIN_ATTACK       = 12,
    BITE_ATTACK             = 13,
    BITE_ATTACK_COOLDOWN    = 14,
    DAMAGED                 = 15,
    DYING                   = 16,
    HIGHLIGHTING            = 17,
};

enum class HighlightingStates
{
    IDLE      = 0,
    BURY_UP   = 1,
    DROP_DOWN = 2,
    WIGGLE    = 3,
    STAND_UP  = 4,
    BURY_DOWN = 5,
};

class Changeling : public Character
{
  public:
    Changeling(GameObject* parent);
    ~Changeling() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;
    void OnPlayerEnterLocation() override;

    void PlayHighlightSequence() override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void UpdateIdleBuriedState(float deltaTime, float distanceToPlayerSq, bool lastAttack);
    void UpdatePeekState(float deltaTime, float distanceToPlayerSq);
    void UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq);
    void UpdateDigDownTransitionState(float deltaTime, float distanceToPlayerSq);
    void UpdateBuriedTravelState(float deltaTime, float distanceToPlayerSq);
    void UpdateIdleVisibleState(float deltaTime, float distanceToPlayerSq, bool lastAttack);
    void UpdateChaseState(float deltaTime, float distanceToPlayerSq, bool lastAttack);
    void UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackWiggleState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashChainAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateBiteAttackCooldownState(float deltaTime, float distanceToPlayerSq);
    void UpdateDamagedState(float deltaTime, float distanceToPlayerSq);
    void UpdateDyingState(float deltaTime, float distanceToPlayerSq);
    void UpdateHighlightState(float deltaTime, float distanceToPlayerSq);

    bool ST_BuryUp(float deltaTime, float distanceToPlayerSq, bool lastAttack);
    bool ST_StartChase(float deltaTime, float distanceToPlayerSq, bool lastAttack);
    bool ST_Damaged();
    bool ST_Peek(float deltaTime, float distanceToPlayerSq);
    bool ST_DashAttack(float deltaTime, float distanceToPlayerSq);
    bool ST_AimNextDashChainAttack(float deltaTime, float distanceToPlayerSq);
    bool ST_AimNextDashAttack(float deltaTime, float distanceToPlayerSq);
    bool ST_BiteAttack(float deltaTime, float distanceToPlayerSq);

  private:
    void ValidateSetup();
    void RenderDebugVisuals();

    // Returns true only if the pooka did not dash against a wall
    bool CalculateDashTargetPoint(const float3& aimingPoint, float3& targetPoint);

    bool ShouldSwapStatesOnRandomVersion(const float deltaTime) const;
    void CalculateAimPoint(float3& outTargetPoint);

    bool isSetupCorrectly               = false;

    AIAgentComponent* agentAI           = nullptr;
    ChangelingStates currentState       = ChangelingStates::NONE;
    ChangelingStates stateAfterDamaged  = ChangelingStates::NONE;

    GameObject* parentGO                = nullptr;

    float4x4 dashStart                  = float4x4::identity;
    float3 dashDirection                = float3::zero;
    float3 dashTarget                   = float3::zero;
    float dashSpeed                     = 15.0f;
    float minDashDistance               = 1;

    std::string dashTrailObjectName     = "DashTrailObject";
    std::string dashTrailStartMeshName  = "DashTrailStartMesh";
    std::string dashTrailMidMeshName    = "DashTrailMidMesh";
    std::string dashTrailEndMeshName    = "DashTrailEndMesh";
    std::string dashTrailCollisionName  = "DashTrailCollision";
    std::string finalAttackColliderName = "FinalAttackObject";

    std::vector<ChangelingDashTrailContainer> dashTrailMeshObjects;
    std::vector<GameObject*> dashTrailColliderObjects;
    std::vector<CubeColliderComponent*> dashAreaColliders;

    float stateTimer                  = 0.f;

    float absoluteSpottedReactionTime = 1.f;
    float biteAttackRadius            = .5f;
    float biteAttackCooldown          = 2.f;
    float activeDashRange             = 0.f;
    bool bNextDashUninterrupted       = false;

    int userSelectedVersion           = 0;
    ChangelingVersions version        = ChangelingVersions::RANDOM;
    float swapStateChancePerSecond    = 0.05f;
    ChangelingVersions randomVersion =
        ChangelingVersions::RANDOM; // How the pooka behaves during this time (Only used if version = 0)
    int maxEnemiesLeftForFinalAttack            = 0;

    HighlightingStates currentHighlightingState = HighlightingStates::IDLE;
    float highlightDuration                     = 3.f;

    float peekChancePerSecond                   = 0.1f;
    float3 spottedLocation                      = float3::nan;
    float buriedTravelSpeed                     = 3.5f;

    // Default specific
    float chaseSpeed                            = 1.0f;
    float chaseAcceleration                     = 4.0f;

    // Block specific
    bool dashRight                              = false;
    unsigned short dashIndex                    = 0;
    float dashAngleDegrees                      = 40.0f;
    float timeBetweenDashes                     = 1.5f;

    // VFX
    // Dig up
    std::string vfxDigUpRocksName               = "VFX_DigUpRocks";
    GameObject* vfxDigUpRocksObject             = nullptr;

    std::string vfxDigUpHoleName                = "VFX_DigUpHole";
    GameObject* vfxDigUpHoleObject              = nullptr;
    
    std::string vfxDashTrailName        = "VFX_DashTrail";
    std::vector<GameObject*> vfxDashTrailObjects;
    
    std::string vfxDropDownName = "VFX_Drop";
    GameObject* vfxDropDown              = nullptr;

    std::string vfxDashName = "VFX_Dash";
    GameObject* vfxDash              = nullptr;
    
    std::string vfxDigDownName = "VFX_DigDown";
    GameObject* vfxDigDown              = nullptr;
    
    std::string vfxBiteName = "VFX_Bite";
    GameObject* vfxBite              = nullptr;
    
    
    // Audio
    AudioSourceComponent* audioComp             = nullptr;
};