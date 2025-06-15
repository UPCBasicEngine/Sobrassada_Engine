#pragma once

#include "Character.h"
#include "Math/float4x4.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ChangelingStates
{
    NONE,
    PATROL,
    CHASE,
    BASIC_ATTACK
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
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;

    void PatrolAI();
    void ChaseAI();

  private:
    float3 GetDashEndPoint() const;

    AIAgentComponent* agentAI     = nullptr;
    ChangelingStates currentState = ChangelingStates::NONE;

    bool isDashing                = false;
    float3 dashDirection          = float3::zero; // Vector dirección normalizado
    float3 dashTarget             = float3::zero; // Posición objetivo
    float dashSpeed               = 15.0f;
    float dashDistance            = 10.0f;

    std::string pathName         = "";

    float3 patrolPoint            = float3::zero;
    bool hasShot                  = false;

    GameObject* pathObj     = nullptr; 

    float3 lastTrailPos           = float3::zero;
    float trailSegmentSpacing     = 1.0f; 
    std::string trailPrefabName   = "DashTrailSegment";
    float4x4 localTransform      = float4x4::identity;

    float3 startPos;
    float3 endPos;

    float3 dashEndPoint = float3::zero; 
};