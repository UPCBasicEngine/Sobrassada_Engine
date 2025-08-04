#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;

enum class SoldierStates
{
    NONE,
    SEARCH,
    PATROL,
    CHASE,
    BASIC_ATTACK,
    DEATH,
    PLAYER_DETECTION
};

class Soldier : public Character
{
  public:
    Soldier(GameObject* parent);
    ~Soldier() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;
    void OnPlayerEnterLocation() override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;

    void ApplyKnockback();

    void ChangeState();
    void PatrolAI(float deltaTime);
    void ChaseAI();
    void SearchForPlayer();
    const char* ManageAttackAnimations();

  private:
    AIAgentComponent* agentAI  = nullptr;
    SoldierStates currentState = SoldierStates::NONE;

    float3 patrolPoint         = float3::zero;

    float knockbackForce       = 7.0f;
    float knockbackTime        = 0.2f;
    float knockbackTimer       = 0.0f;
    float3 knockbackDirection  = float3::zero;
    bool isKnockback           = false;
    int consecutiveAttack     = 0; 
    int consecutiveThrust     = 0; 
    float secondAttackDelay          = 0.6f;
    const char* currentAttackTrigger = nullptr;
    float originalAttackDuration     = 0.0f;
    float originalAttackHitboxDelay  = 0.0f;
    float deathTimer                 = 0.0f;
    float chaseSpeed                = 2.0f;
};