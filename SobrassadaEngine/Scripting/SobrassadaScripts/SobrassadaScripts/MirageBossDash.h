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
    Idle,
    OverheadStrike,
};

enum class BossActions
{
    Idle,
    Dash,
};

class Boss : public Character
{
  public:
    Boss(GameObject* parent);
    ~Boss() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    GameObject* GetCloseArea() const { return closeArea; }
    int GetCloseAreaDamage() const { return closeAreaDamage; }

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

    void OverheadStrike(float deltaTime);
    void StartDash();
    void Dash(float deltaTime);

    const char* GetStateName() const;
    const char* GetActionName() const;

  private:
    BossStates currentState   = BossStates::Idle;
    BossActions currentAction = BossActions::Idle;

    bool stateEnter           = true;
    bool doIdle               = false;
    bool actionTriggerDone    = false;

    // Dash
    bool isDashing            = false;
    float dashSpeed           = 0.0f;
    float dashTimeRemaining   = 0.0f;
    float dashDistance        = 0.0f;
    float3 dashDirection      = float3::zero;
    float3 dashStartPosLocal  = float3::zero;

    float dashDuration        = 0.5f;

};
