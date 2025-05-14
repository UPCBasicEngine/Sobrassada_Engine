#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;

enum class BansheeStates
{
    Idle,
    Chase,
    Flee,
    Scream
};

class Banshee : public Character
{
  public:
    Banshee(GameObject* parent);
    ~Banshee() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;

    void ChasePlayer();
    void Flee();
    void Attack() override;
    void ChangeState();

  private:
    AIAgentComponent* agentAI  = nullptr;
    BansheeStates currentState = BansheeStates::Idle;

    float fleeDistance         = 0.0f;
    float fleeSpeed            = 10.0f;
    bool isFleeing             = false;
};