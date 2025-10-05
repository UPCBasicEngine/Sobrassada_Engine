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

  public:
    // Core AI methods
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void Attack(float deltaTime) override;
    void OverShooting(float deltaTime);
    void Escape(float deltaTime);
    void Aim(float deltaTime);
    void ChangeState();
    void PatrolAI();
    void ChaseAI();
    void SearchForPlayer();
    void ApplyKnockback();
 
 

    // Line of sight and positioning
    bool CheckLineOfSight();
    bool HasLineOfSightFromPosition(float3 fromPos, float3 toPos);
    float3 CalculatePredictiveTarget();
    bool CanShootSafely();

    // Streamlined cover system
    GameObject* FindBestCoverPoint();
    void SeekCover(float deltaTime);
    void StayInCover(float deltaTime);
    void PositionToShoot(float deltaTime);
    float3 FindShootingPosition();

    // Management methods
    void ReleaseCoverPoint();
    void ForceNewCoverPoint();
    GameObject* GetCurrentCoverPoint();
    void DebugCoverPoints();

    // Detection methods
    bool IsPlayerInAnyCoverPoint();
    float3 CalculateSpreadPosition();
    std::vector<float3> GetNearbyArcherPositions();

    // Accessors for CoverPointTrigger
    std::vector<GameObject*>& GetAvailableCoverPoints();
    std::vector<GameObject*>& GetOccupiedCoverPoints();

    //Debug
    const std::string GetLogicStateName();
    const AIAgentComponent* GetAI() { return agentAI; }
    void ActivateGlowVFX();

  private:
    // Core components
    AIAgentComponent* agentAI   = nullptr;
    AudioSourceComponent* audio = nullptr;
    Scene* scene                = nullptr;
    const std::vector<GameObject*>* walls;
    const std::vector<GameObject*>* soldiers;

    // State and basic properties
    ArcherStates currentState = ArcherStates::NONE;
    std::string arrowName     = "";
    float3 patrolPoint        = float3::zero;
    bool isStatic             = false;

    // Combat system
    ArcherProjectile* arrow   = nullptr;
    std::vector<ArcherProjectile*> arrowPool;
    bool hasMultipleShoots     = false;
    int numberOfShoots         = 1;
    int currentArrowIndex      = 0;
    int currentShot            = 0;
    float shotDelay            = 0.2f;
    float shotTimer            = 0.0f;
    bool hasShot               = false;
    bool hasStartedShooting    = false;

    // Aiming system
    bool isAiming              = false;
    float aimTimer             = 0.0f;
    float aimDuration          = 2.0f;

    // Knockback system
    bool isKnockback           = false;
    float knockbackForce       = 7.0f;
    float knockbackTime        = 0.2f;
    float knockbackTimer       = 0.0f;
    float3 knockbackDirection  = float3::zero;

    // Escape system
    float rangeEscape          = rangeAIAttack - 1;
    float3 currentEscapeTarget = float3::zero;
    bool hasEscapeTarget       = false;
    float escapeTimeout        = 0.0f;

    // Streamlined cover system
    std::vector<GameObject*> availableCoverPoints;
    std::vector<GameObject*> occupiedCoverPoints;
    GameObject* currentCoverPoint = nullptr;
    GameObject* currentCover      = nullptr; // Keep for compatibility
    float3 coverPosition          = float3::zero;
    float3 shootingPosition       = float3::zero;
    bool isInCover                = false;
    bool seekingCover             = false;

    // AI behavior tuning
    int flankingFailures          = 0;
    float repositionTimer         = 0.0f;
    float repositionDelay         = 2.0f;
    float lastChaseDistance       = 999.0f;
    float chaseStuckTimer         = 0.0f;
    float losLostTimer            = 0.0f; // Timer para line of sight perdido
    float aimAttemptTimer         = 0.0f;

    // Constants
    float safeShootingDistance    = 12.0f;
    float deathTimer              = 0.0f;
    const float DEATH_DURATION    = 2.0f;

    // VFX
    std::string archerHitVFX      = "HitArcher";
    GameObject* archerVfxObject   = nullptr;
    bool hitVfxIsActive           = false;
    float hitVfxDuration          = 0.5f;
    float hitVfxTimer             = 0.0f;

    //VFX Glow Arrow
    std::string glowHitVFX      = "Glow";
    GameObject* glowVfxObject   = nullptr;
    bool glowVfxIsActive           = false;
    float glowVfxDuration          = 1.0f;
    float glowTimer            =   0.0f;

    // Line of sight tracking
    bool hasLineOfSight           = false;
    int flankingAttempts          = 0;

    //Dash
    bool isDashing                = false;
};