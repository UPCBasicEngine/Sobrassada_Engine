#pragma once

#include "Character.h"

#include <array>

class GameObject;
class AIAgentComponent;
class BossMirage;
class ImageComponent;

enum class BossDashStates
{
    None,
    Idle,
    OverheadStrike,
};

enum class BossDashActions
{
    Idle,
    Dash,
};

class MirageBossDash : public Character
{
  public:
    MirageBossDash(GameObject* parent);
    ~MirageBossDash() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;
    void setState(BossDashStates state) { currentState = state; }
    void setAction(BossDashActions action) { currentAction = action; }
    void setStateBool(bool state) { stateEnter = state; }
    void setEndPoint(float3 endPoint) { dashEnd = endPoint; }

  private:
    void HandleState(float deltaTime) override;

    void Idle();

    void OverheadStrike(float deltaTime);
    void StartDash();
    void Dash(float deltaTime);

    const char* GetStateName() const;
    const char* GetActionName() const;

  private:
    BossDashStates currentState   = BossDashStates::Idle;
    BossDashActions currentAction = BossDashActions::Idle;

    bool stateEnter               = true;
    bool doIdle                   = false;
    bool actionTriggerDone        = false;

    // Dash
    bool isDashing                = false;
    float dashSpeed               = 0.0f;
    float dashTimeRemaining       = 0.0f;
    float dashDistance            = 0.0f;
    float3 dashEnd;
    float3 dashDirection     = float3::zero;
    float3 dashStartPosLocal = float3::zero;

    float dashDuration       = 0.5f;
};
