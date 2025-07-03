#pragma once

#include "Character.h"
#include "Math/float4x4.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ChangelingStates
{
    NONE,
    HIDDEN,
    CHASE,
    DASH_ATTACK
};

class Changeling : public Character
{
  public:
    Changeling(GameObject* parent);
    ~Changeling() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void UpdateHiddenState(float deltaTime);
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;

    void HiddenManagement();
    void ChaseAI();

    void ChangeState();

  private:
    float3 GetDashEndPoint() const;

    AIAgentComponent* agentAI     = nullptr;
    ChangelingStates currentState = ChangelingStates::NONE;

    bool isDashing                = false;
    float3 dashDirection          = float3::zero; // Vector direcci�n normalizado
    float3 dashTarget             = float3::zero; // Posici�n objetivo
    float dashSpeed               = 15.0f;
    float dashDistance            = 10.0f;

    std::string pathName          = "";
    std::string bodyMeshPath      = "";

    bool hasShot                  = false;

    GameObject* dashAreaObject    = nullptr;
    GameObject* bodyMeshObject    = nullptr;

    float3 lastTrailPos           = float3::zero;
    float trailSegmentSpacing     = 1.0f; 
    std::string trailPrefabName   = "DashTrailSegment";
    float4x4 localTransform      = float4x4::identity;

    float3 startPos;
    float3 endPos;

    float3 dashEndPoint = float3::zero;
};