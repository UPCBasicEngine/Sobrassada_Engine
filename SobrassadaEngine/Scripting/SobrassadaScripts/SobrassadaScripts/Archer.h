#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ArcherStates
{
    NONE,
    PATROL,
    CHASE,
    BASIC_ATTACK
};

class Archer : public Character
{
  public:
    Archer(GameObject* parent);
    ~Archer() noexcept override { parent = nullptr; };

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
    AIAgentComponent* agentAI = nullptr;
    ArcherStates currentState = ArcherStates::NONE;

    Projectile* arrow         = nullptr;

    float3 patrolPoint        = float3::zero;
    bool hasShot              = false;
};