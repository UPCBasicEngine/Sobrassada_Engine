#pragma once

#include "Character.h"

class GameObject;
class AIAgentComponent;
class AudioSourceComponent;

enum class SoldierStates
{
    NONE,
    SEARCH,
    PATROL,
    CHASE,
    BASIC_ATTACK,
    DEATH,
    PLAYER_DETECTION,
    CHEERING
};

class Soldier : public Character
{
  public:
    Soldier(GameObject* parent);
    ~Soldier() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;
    void OnPlayerEnterLocation() override;
    void SetAttackVFX();
    void DisableAttackVFX();

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;

    void ApplyKnockback();

    void ChangeState();
    void PatrolAI(float deltaTime);
    void ChaseAI();
    void SearchForPlayer();
    void SetOnWaiting();
    const char* ManageAttackAnimations();
    void SelectRandomHelmet();

  private:
    AIAgentComponent* agentAI        = nullptr;
    SoldierStates currentState       = SoldierStates::NONE;

    AudioSourceComponent* audio      = nullptr;

    float3 patrolPoint               = float3::zero;

    float knockbackForce             = 7.0f;
    float knockbackTime              = 0.2f;
    float knockbackTimer             = 0.0f;
    float3 knockbackDirection        = float3::zero;
    bool isKnockback                 = false;
    bool isStrongKnockback           = false;
    int consecutiveAttack            = 0;
    int consecutiveThrust            = 0;
    float secondAttackDelay          = 0.6f;
    const char* currentAttackTrigger = nullptr;
    float originalAttackDuration     = 0.0f;
    float originalAttackHitboxDelay  = 0.0f;
    float deathTimer                 = 0.0f;
    float chaseSpeed                 = 2.0f;
    bool thrustAdvance               = false;
    bool countedInPlayerEnemies      = false;
    float cheeringDistance           = 5.0f;

    int maxEnemiesNearby   = 3;
    std::string meleeTrailName       = "";
    GameObject* meleeTrailObject     = nullptr;
    std::string helmet1Name       = "";
    GameObject* helmet1Object     = nullptr;
    std::string helmet2Name          = "";
    GameObject* helmet2Object        = nullptr;
    std::string helmet3Name          = "";
    GameObject* helmet3Object        = nullptr;
    std::string helmet4Name          = "";
    GameObject* helmet4Object        = nullptr;
    std::string meleeVfxName         = "";
    GameObject* meleeVfxObject       = nullptr;
    std::string thrustVfxName         = "";
    GameObject* thrustVfxObject       = nullptr;
};