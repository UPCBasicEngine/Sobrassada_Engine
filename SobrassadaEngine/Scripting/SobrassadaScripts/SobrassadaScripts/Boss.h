#pragma once

#include "Character.h"

#include <array>
#include <random>

class GameObject;
class AIAgentComponent;

enum class BossStates
{
    None,
    Idle,
    Taunt,
    ShieldStrikes,
    OverheadStrike,
    Mirage,
    WaterSpouts,
};

enum class BossActions
{
    Idle,
    Taunt,
    Chase,
    Combo1,
    Combo2,
    Combo3,
    Prepare,
    Jump,
    Dash,
    Land,
    Attack,
    Recover,
    Mirage,
    WaterSpouts,
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
    void Taunt(float deltaTime);
    void ShieldStrikes(float deltaTime);
    void OverheadStrike(float deltaTime);
    void Mirage();

    const char* GetStateName() const;
    const char* GetActionName() const;

  public:
    bool activateMirage = false;

  private:
    AIAgentComponent* agentAI           = nullptr;
    BossStates currentState             = BossStates::Idle;
    BossActions currentAction           = BossActions::Idle;

    int phase                           = 1;
    std::array<int, 3> phaseSwap        = {40, 20, 0};
    bool stateEnter                     = true;
    bool doIdle                         = false;
    bool doTaunt                        = false;
    bool actionTriggerDone              = false;

    int shieldStrikeLastAction          = 0;

    bool mirageActivated                = false;

    std::array<int, 3> mirageActivation = {50, 30, 10};

    /* Melee */
    float shieldStrikesRange            = 5.0f;
    float overheadStrikeRange           = 5.0f;

    std::mt19937 rng;
    std::uniform_int_distribution<int> uniformDist;
};
