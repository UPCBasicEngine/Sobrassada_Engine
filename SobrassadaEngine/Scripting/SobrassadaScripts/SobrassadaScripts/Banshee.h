#pragma once

#include "Character.h"

#include "Math/float2.h"
#include "Math/float4.h"
#include "imgui.h"
#include <random>
#include <vector>

class GameObject;
class MeshComponent;
class AIAgentComponent;
class SphereColliderComponent;
class CapsuleColliderComponent;
class ShaderScriptComponent;
class ResourceMaterial;
class ParticleSystemComponent;
class AudioSourceComponent;

enum class BansheeStates : int
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

constexpr const char* BansheeStateStrings[] = {"Idle", "Search", "Chase",          "Attack",
                                               "Hit",  "Dead",   "TeleportOrigin", "SlowArea"};

class Banshee : public Character
{
  public:
    Banshee(GameObject* parent);
    ~Banshee() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;

    BansheeStates GetState() const { return currentState; }
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
    void MoveSlowAreaToPlayer();
    void UpdateLastPlayerPosition();

  private:
    float2 invisibleTimeRange  = float2::zero;
    float currentInvisibleTime = 0.0f;
    float attackAngularSpeed   = 0.0f;
    bool isInvisible           = false;

    float teleportVFXDuration  = 0.5f;
    float elapsedTeleportVFX   = 0.0f;
    bool teleportedToPos       = false;

    bool playedDeathSound      = false;

    float warningDuration      = 0.2f;
    float elapsedWarning       = 0.f;

    float mainScreamDuration   = 2.f;
    float elapsedMainScream    = 0.f;

    AIAgentComponent* agentAI  = nullptr;
    BansheeStates currentState = BansheeStates::Idle;
    MeshComponent* mesh        = nullptr;

    std::mt19937 rng;
    std::uniform_real_distribution<float> normalizedDist;
    std::uniform_real_distribution<float> invisibleDist;

    bool firstSearch                    = false;
    bool hasMoved                       = false;

    GameObject* screamAreaWarningGO     = nullptr;

    GameObject* slowAreaGO              = nullptr;
    GameObject* slowAreaInGO            = nullptr;
    GameObject* slowAreaWarningGO       = nullptr;
    ShaderScriptComponent* slowAreaRing = nullptr;

    int slowAreaDamage                  = 1;
    float slowAreaWaringDuration        = 1.f;
    float elapsedSlowAreaWaring         = 0.f;
    float slowAreaWaringMaxScale        = 5.f;

    float elapsedSlowArea               = 0.f;
    float slowAreaDuration              = 1.f;

    float slowAreaStartHeight           = 0.5f;
    float slowAreaInStartHeight         = 0.45f;
    float slowWarningStartHeight        = 0.45f;
    float slowRingStartHeight           = 0.40f;

    std::vector<ShaderScriptComponent*> forwardScreamShaderComponents;
    CapsuleColliderComponent* forwardScreamCollider = nullptr;

    GameObject* groundRing                          = nullptr;
    std::vector<ShaderScriptComponent*> groundRingShaderComponents;

    ImVec2 curveEditorPoints[StoreScriptCurvePoints];

    GameObject* teleportWarningScreamGO = nullptr;
    GameObject* teleportWarningSlowGO   = nullptr;
    std::vector<ShaderScriptComponent*> teleportVFXShaderComponents;

    const float4 screamWarningColor              = float4(0.89f, 0.243f, 0.243f, 1.f);
    const float4 slowWarningColor                = float4(0.243f, 0.369f, 0.89f, 1.f);

    float3 lastPlayerPosition                    = float3::zero;

    // ParticleSystemComponent* hitParticleSystem = nullptr;
    ParticleSystemComponent* chaseParticleSystem = nullptr;
    std::vector<ShaderScriptComponent*> hitVFXShaderComponents;
    std::vector<ShaderScriptComponent*> deathVFXShaderComponents;

    AudioSourceComponent* audioSource = nullptr;
};
