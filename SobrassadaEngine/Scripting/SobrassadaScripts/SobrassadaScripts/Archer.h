#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;
class Projectile;

enum class ArcherStates
{
    NONE,
    SEARCH,
    PATROL,
    CHASE,
    ESCAPE,
    AIM,
    BASIC_ATTACK,
    DEATH,
    OVERSHOOTING
};

class Archer : public Character
{
  public:
    Archer(GameObject* parent);
    ~Archer() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;
    void OnPlayerEnterLocation() override;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void OverShooting(float deltaTime);
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;
    void Escape(float deltaTime);
    void Aim(float deltaTime);

    void ChangeState();
    void PatrolAI();
    void ChaseAI();
    void SearchForPlayer();
    void ApplyKnockback();

  private:
    float rangeEscape           = rangeAIAttack - 1;
    AIAgentComponent* agentAI   = nullptr;
    ArcherStates currentState   = ArcherStates::NONE;

    std::string arrowName       = "";
    Projectile* arrow           = nullptr;

    float3 patrolPoint          = float3::zero;
    bool hasShot                = false;

    float3 currentEscapeTarget  = float3::zero;
    bool hasEscapeTarget        = false;
    float knockbackForce        = 7.0f;
    float knockbackTime         = 0.2f;
    float knockbackTimer        = 0.0f;
    float3 knockbackDirection   = float3::zero;
    bool isKnockback            = false;

    bool isAiming               = false;
    bool hasMultipleShoots      = false;
    bool isStatic               = false;

    int numberOfShoots          = 1;
    float aimTimer              = 0.0f;
    float aimDuration           = 2.0f;
    float deathTimer            = 0.0f;
    const float DEATH_DURATION  = 2.0f;

    int currentShot             = 0;
    float shotDelay             = 0.2f;
    float shotTimer             = 0.0f;
    bool hasStartedShooting     = false;

    float escapeUsedTimer       = 0.0f;
    const float ESCAPE_COOLDOWN = 8.0f;
    int escapeCount             = 0;
    const int MAX_ESCAPES       = 2;
    bool canEscape              = true;

    std::vector<Projectile*> arrowPool; 
    int currentArrowIndex = 0;          
    int poolSize          = 5;          
};