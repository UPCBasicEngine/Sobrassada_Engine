#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;

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

  private:
    AIAgentComponent* agentAI = nullptr;
};