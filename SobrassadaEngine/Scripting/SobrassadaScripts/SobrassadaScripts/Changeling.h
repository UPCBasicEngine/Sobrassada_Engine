#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ChangelingStates
{
    NONE,
    PATROL,
    CHASE,
    BASIC_ATTACK
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
    void Attack(float deltaTime) override;

    void PatrolAI();
    void ChaseAI();

  private:
    AIAgentComponent* agentAI     = nullptr;
    ChangelingStates currentState = ChangelingStates::NONE;

    bool isDashing                = false;
    float3 dashDirection          = float3::zero; // Vector dirección normalizado
    float3 dashTarget             = float3::zero; // Posición objetivo
    float dashSpeed               = 15.0f;
    float dashTimeRemaining       = 0.0f;
    float dashDuration            = 0.2f;

    float3 patrolPoint            = float3::zero;
    bool hasShot                  = false;
};