#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ArcherStates
{
    NONE,
    SEARCH,
    PATROL,
    CHASE,
    ESCAPE,
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
    void Escape(float deltaTime);

    void ChangeState();
    void PatrolAI();
    void ChaseAI();
    void SearchForPlayer();

  private:
    float rangeEscape          = rangeAIAttack - 1;
    AIAgentComponent* agentAI  = nullptr;
    ArcherStates currentState  = ArcherStates::NONE;

    std::string arrowName      = "";
    Projectile* arrow          = nullptr;

    Projectile* arrow         = nullptr;
    float3 patrolPoint         = float3::zero;
    bool hasShot               = false;

    float3 currentEscapeTarget = float3::zero;
    bool hasEscapeTarget       = false;
};