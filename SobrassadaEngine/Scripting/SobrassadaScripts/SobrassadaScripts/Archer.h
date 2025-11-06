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
    DANGER, 
    HIGHLIGHTING,
    PREAIM
};

enum class ArcherHighlightingStates
{
    IDLE      = 0,
    AIM = 2,
    BASIC_ATTACK = 3,
    COOLDOWN = 4,
    DONE = 5
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
    void PlayHighlightSequence() override;
    void UpdateHighlightState(float deltaTime);

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
    void PreAim(float deltaTime);
    void ChangeState();
    void PatrolAI();
    void ChaseAI();
    void DangerRetreat(float deltaTime);
    void SearchForPlayer();
    void ApplyKnockback();
    bool IsNavmeshPathClear(float3 from, float3 to);
 
 

    // Line of sight and positioning
    bool CheckLineOfSight();
    bool HasLineOfSightFromPosition(float3 fromPos, float3 toPos);
    float3 CalculatePredictiveTarget();
    bool CanShootSafely();

   

  

    // Detection methods
    float3 CalculateSpreadPosition();
    std::vector<float3> GetNearbyArcherPositions();



    //Debug
    const std::string GetLogicStateName();
    const AIAgentComponent* GetAI() { return agentAI; }
    void ActivateGlowVFX();
    void ActivateHitVFX();

   

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
    ArcherHighlightingStates currentHighlightingState = ArcherHighlightingStates::IDLE;
    float stateTimer                            = 0.f;
    float highlightDuration                     = 3.f;


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
    float breathTime           = 0.5;
    float breathDuration       = 0.0f;
    bool shouldAttack = true;

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
    GameObject* currentCover      = nullptr; 
    float3 coverPosition          = float3::zero;
    float3 shootingPosition       = float3::zero;
    bool isInCover                = false;
    bool seekingCover             = false;

    // AI behavior tuning
    int flankingFailures          = 0;
    float repositionTimer         = 0.0f;
    float repositionDelay         = 2.0f;
    float3 targetSpreadPosition        = float3::zero;
    bool hasSpreadPosition             = false;
   
    

    // Constants
    float safeShootingDistance    = 12.0f;
    float deathTimer              = 0.0f;
    const float DEATH_DURATION    = 2.0f;

    // VFX
    std::string archerHitVFX      = "HitArcher";
    GameObject* hitVfxObject   = nullptr;
    bool hitVfxIsActive           = false;
    float hitVfxDuration          = 0.5f;
    float hitVfxTimer             = 0.0f;

    //VFX Glow Arrow
    std::string glowHitVFX      = "Glow";
    GameObject* glowVfxObject   = nullptr;
    bool glowVfxIsActive           = false;
    float glowVfxDuration          = 1.0f;
    float glowTimer            =   0.0f;

    
    float preAimDuration           = 0.8f; 
    float preAimTimer              = 0.0f;
    bool isPreAiming               = false;
   
    //Danger Zone
    float dangerTimer                  = 0.0f;
    float dangerDuration               = 2.0f; 
    float3 dangerEscapeTarget          = float3::zero;
    bool hasDangerTarget               = false;
    

    // Line of sight tracking
    bool hasLineOfSight           = false;
    int flankingAttempts          = 0;

    //Dash
    bool isDashing                = false;
    

    static bool triggered;
  float3 lastDangerPosition          = float3::zero;
  float dangerStuckTimer             = 0.0f;
  float highlightTimer               = 0.0f;


};