#pragma once
#include "Character.h"

class GameObject;
class AIAgentComponent;
class ArcherProjectile;
class AudioSourceComponent;
class Scene;

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
    OVERSHOOTING,
    SEEKING_COVER,
    IN_COVER,
    POSITIONING_TO_SHOOT
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
    float3 CalculatePredictiveTarget();
    void ChangeState();
    void PatrolAI();
    void ChaseAI();
    void SearchForPlayer();
    void ApplyKnockback();

    bool CheckLineOfSight();
    bool ShouldSeekCover();
    bool HasNearbyAllies();
    GameObject* FindNearestCover();
    float CalculateCoverScore(GameObject* coverObj);
    float3 FindShootingPosition();
    float3 FindClearShootingPosition();
    bool CanShootSafely();
    bool HasLineOfSightFromPosition(float3 fromPos, float3 toPos);
    void SeekCover(float deltaTime);
    void StayInCover(float deltaTime);
    void PositionToShoot(float deltaTime);
    const std::string GetLogicStateName();

  private:
    float rangeEscape          = rangeAIAttack - 1;
    AIAgentComponent* agentAI  = nullptr;
    ArcherStates currentState  = ArcherStates::NONE;
    std::string arrowName      = "";
    ArcherProjectile* arrow    = nullptr;
    float3 patrolPoint         = float3::zero;
    bool hasShot               = false;
    float3 currentEscapeTarget = float3::zero;
    bool hasEscapeTarget       = false;
    float knockbackForce       = 7.0f;
    float knockbackTime        = 0.2f;
    float knockbackTimer       = 0.0f;
    float3 knockbackDirection  = float3::zero;
    bool isKnockback           = false;
    bool isAiming              = false;
    bool hasMultipleShoots     = false;
    bool isStatic              = false;
    int numberOfShoots         = 1;
    float aimTimer             = 0.0f;
    float aimDuration          = 2.0f;
    float deathTimer           = 0.0f;
    const float DEATH_DURATION = 2.0f;
    int currentShot            = 0;
    float shotDelay            = 0.2f;
    float shotTimer            = 0.0f;
    bool hasStartedShooting    = false;
    std::vector<ArcherProjectile*> arrowPool;
    int currentArrowIndex       = 0;
    int poolSize                = 5;

    bool hasLineOfSight         = false;
    float chaseTimer            = 0.0f;
    float maxChaseTime          = 8.0f;
    float lastDistanceToPlayer  = 999.0f;
    float stuckThreshold        = 1.0f;

    bool isInCover              = false;
    bool seekingCover           = false;
    GameObject* currentCover    = nullptr;
    float3 coverPosition        = float3::zero;
    float timeInCover           = 0.0f;

    float3 shootingPosition     = float3::zero;
    float coverSeekRange        = 8.0f;
    float coverRadius           = 3.0f;
    float safeShootingDistance  = 12.0f;
    float repositionTimer       = 0.0f;
    float repositionDelay       = 2.0f;
    float allyDetectionRange    = 6.0f;

    float repositionTimeout     = 2.0f;
    bool isRepositioning        = false;
    float3 repositionTarget     = float3::zero;
    AudioSourceComponent* audio = nullptr;
    const std::vector<GameObject*>* walls;
    const std::vector<GameObject*>* soldiers;
    Scene* scene                = nullptr;

    // Hit VFX
    std::string archerHitVFX    = "HitArcher";
    GameObject* archerVfxObject = nullptr;
    bool hitVfxIsActive         = false;
    float hitVfxDuration        = 0.8f;
    float hitVfxTimer           = 0.0f;
};