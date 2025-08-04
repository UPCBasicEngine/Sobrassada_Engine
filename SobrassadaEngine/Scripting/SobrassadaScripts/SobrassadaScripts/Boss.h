#pragma once

#include "Character.h"

#include <array>
#include <random>

class GameObject;
class AIAgentComponent;
class BossMirage;
class ImageComponent;

enum class BossStates
{
    None,
    ChangePhase,
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
    ChangeStart,
    ChangeCharge,
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
    void ChooseNextStateFirstPhase();
    void ChooseNextStateSecondPhase();
    void ChooseNextStateThirdPhase();

    void Idle();
    void Taunt(float deltaTime);
    void ShieldStrikes(float deltaTime);
    void OverheadStrike(float deltaTime);
    void Mirage();

    const char* GetStateName() const;
    const char* GetActionName() const;

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
    std::array<int, 3> mirageActivation = {mirage1, mirage2, mirage3};

    /* Melee */
    float shieldStrikesRange            = 5.0f;
    float overheadStrikeRange           = 5.0f;

    std::mt19937 rng;
    std::uniform_int_distribution<int> uniformDist;
    BossMirage* bossMirageScript    = nullptr;

    ImageComponent* healthImageComponent = nullptr;
    UID healthBarImage;
    int health = 0;
    int mirage1, mirage2, mirage3 = 0;
    int currentMirage = 0;

};
