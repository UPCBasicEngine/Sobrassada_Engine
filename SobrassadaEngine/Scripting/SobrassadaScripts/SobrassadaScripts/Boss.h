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
    void StartDash();
    void Dash(float deltaTime);
    void StartJump(bool fall = false);
    void Jump(float deltaTime, bool fall = false);

    void Mirage();

    const char* GetStateName() const;
    const char* GetActionName() const;

  private:
    AIAgentComponent* agentAI = nullptr;
    BossStates currentState   = BossStates::Idle;
    BossActions currentAction = BossActions::Idle;

    int phase                 = 1;
    int phase1 = 40, phase2 = 20, phase3 = 0;
    std::array<int, 3> phaseSwap = {phase1, phase2, phase3};
    bool stateEnter              = true;
    bool doIdle                  = false;
    bool doTaunt                 = false;
    bool actionTriggerDone       = false;

    int shieldStrikeLastAction   = 0;

    float3 startPosLocal         = float3::zero;

    // Dash
    bool isDashing               = false;
    float dashSpeed              = 0.0f;
    float dashTimeRemaining      = 0.0f;
    float dashDuration           = 0.5f;
    float dashDistance           = 0.0f;
    float3 dashDirection         = float3::zero;

    // Jump
    bool isJumping               = false;
    float jumpSpeed              = 0.0f;
    float jumpTimeRemaining      = 0.0f;

    float heightJump             = 4.0f;
    float jumpDuration           = 0.2f;
    float fallDuration           = 0.2f;

    std::mt19937 rng;
    std::uniform_int_distribution<int> uniformDist;

    ImageComponent* healthImageComponent = nullptr;
    UID healthBarImage;
    int health  = 0;

    int mirage1 = 50, mirage2 = 30, mirage3 = 10;
    std::array<int, 3> mirageActivation = {mirage1, mirage2, mirage3};
    BossMirage* bossMirageScript        = nullptr;
    bool mirageActivated                = false;
    int currentMirage                   = 0;
};
