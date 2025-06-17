#pragma once

#include "Character.h"

#include "Math/float2.h"
#include <random>

class GameObject;
class MeshComponent;
class AIAgentComponent;
class SphereColliderComponent;

enum class BansheeStates
{
    Idle,
    Chase,
    Attack
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
    void Attack(float deltaTime) override;
    void ChangeState();
    void GoToAttackPosition();

    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

  private:
    AIAgentComponent* agentAI           = nullptr;
    BansheeStates currentState          = BansheeStates::Idle;
    SphereColliderComponent* damageArea = nullptr;
    GameObject* areaVisual              = nullptr;
    GameObject* screamVisual            = nullptr;
    MeshComponent* mesh                 = nullptr;

    float2 invisibleTimeRange           = float2::zero;
    float currentInvisibleTime          = 0.0f;
    float attackAngularSpeed            = 0.0f;
    bool isInvisible                    = false;

    std::mt19937 rng;
    std::uniform_real_distribution<float> normalizedDist;
    std::uniform_real_distribution<float> invisibleDist;
};