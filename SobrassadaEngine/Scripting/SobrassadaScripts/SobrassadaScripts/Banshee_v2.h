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

enum class Banshee_v2_States : int
{
    Idle = 0,
    Search,
    Chase,
    Attack,
    Hit,
    Dead,
    TeleportOrigin,
    SlowArea,
};

constexpr const char* Banshee_v2_StateStrings[] = {"Idle", "Search", "Chase",          "Attack",
                                                   "Hit",  "Dead",   "TeleportOrigin", "SlowArea"};

class Banshee_v2 : public Character
{
  public:
    Banshee_v2(GameObject* parent);
    ~Banshee_v2() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;

    Banshee_v2_States GetState() const { return currentState; }
    int GetSlowAreaDamage() const { return slowAreaDamage; }

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
    void SlowArea(float deltaTime);

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
    Banshee_v2_States currentState = Banshee_v2_States::Idle;
    MeshComponent* mesh            = nullptr;

    std::mt19937 rng;
    std::uniform_real_distribution<float> normalizedDist;
    std::uniform_real_distribution<float> invisibleDist;

    bool firstSearch                   = false;
    bool hasMoved                      = false;

    AnimationComponent* shoutStartAnim = nullptr;
    AnimationComponent* shoutBaseAnim  = nullptr;

    GameObject* slowAreaGO             = nullptr;
    GameObject* slowAreaWarningGO      = nullptr;
    int slowAreaDamage                 = 1;
    float slowAreaWaringDuration       = 1.f;
    float elapsedSlowAreaWaring        = 0.f;
    float slowAreaWaringMaxScale       = 5.f;

    std::vector<ShaderScriptComponent*> shoutStartShaderComponents;
    std::vector<ShaderScriptComponent*> shoutBaseShaderComponents;

    std::vector<MeshComponent*> shoutStartMeshComponents;
    std::vector<MeshComponent*> shoutBaseMeshComponents;
};
