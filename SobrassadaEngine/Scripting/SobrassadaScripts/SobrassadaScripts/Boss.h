#pragma once

#include "Character.h"

#include <random>

class GameObject;
class AIAgentComponent;

enum class BossStates
{
    Movement,
    ShieldStrikes,
    ShieldThrow,
    WaterSpouts,
    Mirage,
    OverheadStrike
};

enum class BossActions
{
    Idle,
    Approach,
    Surround,
    JumpAway,
    Chase,
    ShieldStrikes,
    ShieldThrow,
    WaterSpouts,
    Mirage,
    OverheadStrike
};

class Boss : public Character
{
  public:
    Boss(GameObject* parent);
    ~Boss() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void HandleState(float deltaTime) override;
    void UpdateTimers(float deltaTime) override;
    void ChooseNextState();

    void Idle();
    void Approach();
    void Surround();
    void JumpAway();
    void Chase();

    void ShieldStrikes(float deltaTime);
    void ShieldThrow();

    const char* GetStateName() const;
    const char* GetActionName() const;

  private:
    AIAgentComponent* agentAI    = nullptr;
    BossStates currentState      = BossStates::Movement;
    BossActions currentAction    = BossActions::Idle;

    int phase                    = 1;
    bool stateEnter              = true;

    /* Movement */
    float movementTimer          = 0.0f;
    float walkSpeed              = 5.0f;
    float chaseSpeed             = 10.0f;

    /* Melee */
    float shieldStrikesRange     = 5.0f;
    float overheadStrikeRange    = 5.0f;

    /* Shield throw */
    float shieldThrowMinDistance = 3.0f;

    std::mt19937 rng;
    std::uniform_int_distribution<int> uniformDist;
};
