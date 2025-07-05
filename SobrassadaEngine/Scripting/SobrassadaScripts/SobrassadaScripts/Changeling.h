#pragma once

#include "Character.h"
#include "Math/float4x4.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ChangelingVersions
{
    RANDOM,
    SEPP,
    HERBERT,
    GIACOMO
};

enum class ChangelingStates
{
    NONE,
    HIDDEN,
    DIG_UP_TRANSITION,
    DIG_DOWN_TRANSITION,
    CHASE,
    DASH_ATTACK_PREPARATION,
    DASH_ATTACK,
    DASH_ATTACK_COOLDOWN,
    BITE_ATTACK,
    BITE_ATTACK_COOLDOWN,
    DYING,
};

class Changeling : public Character
{
  public:
    Changeling(GameObject* parent);
    ~Changeling() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void UpdateHiddenState(float deltaTime, float distanceToPlayerSq);
    void UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq);
    void UpdateDigDownTransitionState(float deltaTime, float distanceToPlayerSq);
    void UpdateChaseState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq);
    void UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq);
    void UpdateBiteAttackCooldownState(float deltaTime, float distanceToPlayerSq);
    void UpdateDyingState(float deltaTime, float distanceToPlayerSq);
    
    bool ST_DashAttack(float deltaTime, float distanceToPlayerSq);
    bool ST_BiteAttack(float deltaTime, float distanceToPlayerSq);

  private:

    void ValidateSetup();
    void RenderDebugVisuals();

    bool isSetupCorrectly = false;
    
    AIAgentComponent* agentAI     = nullptr;
    ChangelingStates currentState = ChangelingStates::NONE;

    float3 dashStart              = float3::zero;
    float3 dashDirection          = float3::zero;
    float3 dashTarget             = float3::zero;
    float dashSpeed               = 15.0f;

    std::string pathName;
    std::string bodyMeshPath;

    GameObject* dashAreaObject    = nullptr;
    GameObject* bodyMeshObject    = nullptr;

    bool hasPlayerSpotted = false;
    float stateTimer = 0.f;
    
    float absoluteSpottedReactionTime = 1.f;
    float absoluteRiseDuration = 1.f;
    float dashAttackPreparationDuration = 1.f;
    float biteAttackRadius = .5f;
    float biteAttackDuration = .5f;
    float biteAttackCooldown = 2.f;

    float dyingDuration = 2.f;

    int userSelectedVersion = 0;
    ChangelingVersions version = ChangelingVersions::RANDOM;
    
    // Herbert specific (default changeling)
    // Sepp specific
    float maxSneakSpeed = 15.0f;
    // Giacomo specific
    
};