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
    BASIC_ATTACK
};

class Soldier : public Character
{
  public:
    Soldier(GameObject* parent);
    ~Soldier() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;

    void ApplyKnockback(const float3& sourcePosition);

    void ChangeState();
    void PatrolAI();
    void ChaseAI();
    void SearchForPlayer();

  private:
    AIAgentComponent* agentAI  = nullptr;
    SoldierStates currentState = SoldierStates::NONE;

    float3 patrolPoint         = float3::zero;

    float knockbackForce = 0.5f;
    float knockbackTime = 0.5f;
    float knockbackTimer = 0.0f;
    bool isKnockback = false;
};