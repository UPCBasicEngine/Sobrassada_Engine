#pragma once

#include "Character.h"
#include "Standalone/Physics/CubeColliderComponent.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ChangelingVersions
{
    RANDOM,
    SEPP,
    HERBERT,
    FRANZ,
};

enum class ChangelingStates
{
    NONE,
    IDLE_BURIED,
    PEEK,
    DIG_UP_TRANSITION,
    DIG_DOWN_TRANSITION,
    IDLE_VISIBLE,
    CHASE,
    BURIED_CHASE,
    DASH_ATTACK_PREPARATION,
    DASH_ATTACK,
    DASH_ATTACK_WIGGLE,
    DASH_ATTACK_COOLDOWN,
    DASH_CHAIN_ATTACK,
    BITE_ATTACK,
    BITE_ATTACK_COOLDOWN,
    DAMAGED,
    DYING,
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

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void UpdateIdleBuriedState(float deltaTime, float distanceToPlayerSq);
    void UpdatePeekState(float deltaTime, float distanceToPlayerSq);
    void UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq);
    void UpdateDigDownTransitionState(float deltaTime, float distanceToPlayerSq);
    void UpdateIdleVisibleState(float deltaTime, float distanceToPlayerSq);
    void UpdateChaseState(float deltaTime, float distanceToPlayerSq);
    void UpdateBuriedChaseState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackWiggleState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashChainAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateBiteAttackCooldownState(float deltaTime, float distanceToPlayerSq);
    void UpdateDamagedState(float deltaTime, float distanceToPlayerSq);
    void UpdateDyingState(float deltaTime, float distanceToPlayerSq);

    bool ST_StartChase(float deltaTime, float distanceToPlayerSq);
    bool ST_StartBuriedChase(float deltaTime, float distanceToPlayerSq);
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

    bool isSetupCorrectly = false;
    
    AIAgentComponent* agentAI     = nullptr;
    ChangelingStates currentState = ChangelingStates::NONE;

    GameObject* parentGO          = nullptr;

    float4x4 dashStart            = float4x4::identity;
    float3 dashDirection          = float3::zero;
    float3 dashTarget             = float3::zero;
    float dashSpeed               = 15.0f;
    float minDashDistance         = 1;

    std::string dashTrailMeshName;
    std::string dashTrailCollisionName;

    std::vector<GameObject*> dashTrailMeshObjects;
    std::vector<GameObject*> dashTrailColliderObjects;
    std::vector<CubeColliderComponent*> dashAreaColliders;

    bool hasPlayerSpotted = false;
    float stateTimer = 0.f;
    
    float absoluteSpottedReactionTime = 1.f;
    float biteAttackRadius = .5f;
    float biteAttackCooldown = 2.f;
    float activeDashRange = 0.f;
    bool bNextDashInterrupted = false;

    int userSelectedVersion = 0;
    ChangelingVersions version = ChangelingVersions::RANDOM;
    float swapStatesRandomlyPercentage = 5.0f;
    ChangelingVersions randomVersion = ChangelingVersions::RANDOM; // How the pooka behaves during this time (Only used if version = 0)
    
    // Sepp specific (default changeling)
    float chaseSpeed = 1.0f;
    float chaseAcceleration = 4.0f;
    
    // Herbert specific
    float maxSneakAngleDegrees = 45.0f;
    float minSneakSpeed = 0.25f;
    float maxSneakSpeed = 1.0f;
    float distanceToPlayerForMaxSneakSpeed = 0.0f;
    float sneakAcceleration = 4.0f;
    float peekChancePerSecond = 0.1f;
    
    // Franz specific
    bool dashRight = false;
    unsigned short dashIndex = 0;
    float dashAngleDegrees = 40.0f;
    float timeBetweenDashes = 2.f;
};