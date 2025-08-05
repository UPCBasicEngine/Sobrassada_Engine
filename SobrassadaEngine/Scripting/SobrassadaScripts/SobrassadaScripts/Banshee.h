#pragma once

#include "Character.h"

#include "Math/float2.h"
#include <random>
#include <vector>

class GameObject;
class MeshComponent;
class AIAgentComponent;
class SphereColliderComponent;
class ShaderScriptComponent;

enum class BansheeStates : int
{
    Idle = 0,
    Search,
    Chase,
    Attack,
    Hit,
    Dead,
    TeleportOrigin,
};

constexpr const char* BansheeStateStrings[] = {"Idle", "Search", "Chase", "Attack", "Hit", "Dead", "TeleportOrigin"};

class Banshee : public Character
{
  public:
    Banshee(GameObject* parent);
    ~Banshee() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void TakeDamage(int amount) override;

    void ChasePlayer();
    void Attack(float deltaTime) override;
    void ChangeState();
    void SearchForPlayer();
    void GoToAttackPosition();
    void TeleportToOrigin();
    void HandleDeath();

  private:
    float2 invisibleTimeRange      = float2::zero;
    float currentInvisibleTime     = 0.0f;
    float attackAngularSpeed       = 0.0f;
    bool isInvisible               = false;

    float warningDuration          = 0.2f;
    float elapsedWarning           = 0.f;

    float mainScreamDuration       = 2.f;
    float elapsedMainScream        = 0.f;

    AIAgentComponent* agentAI      = nullptr;
    BansheeStates currentState     = BansheeStates::Idle;
    MeshComponent* mesh            = nullptr;

    MeshComponent* meshWarningStar = nullptr;

    std::mt19937 rng;
    std::uniform_real_distribution<float> normalizedDist;
    std::uniform_real_distribution<float> invisibleDist;

    bool firstSearch = false;
    bool hasMoved    = false;

    std::vector<ShaderScriptComponent*> shoutStartComponents;
    std::vector<ShaderScriptComponent*> shoutBaseComponents;
};