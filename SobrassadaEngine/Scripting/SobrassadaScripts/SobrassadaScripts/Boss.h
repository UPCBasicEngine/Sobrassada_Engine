#pragma once

#include "Character.h"

#include <array>
#include <random>

class GameObject;
class AIAgentComponent;
class BossMirage;
class ImageComponent;
class ShaderScriptComponent;
class MovingUVTransparent;
class MeshComponent;
class ParticleSystemComponent;
class CapsuleColliderComponent;
class Spouts;

enum class BossDistance
{
    Close,   // 3m
    Near,    // 4m
    Medium,  // 6.5m
    Distant, // 8.5m
    Far,     // 10m
    Farther, // 13m
    Extreme  // 15m
};

enum class BossStates
{
    None,
    Idle,
    Taunt,
    ShieldStrikes,
    OverheadStrike,
    Mirage,
    ChangePhase,
    WaterSpouts,
    ShieldBlast,
};

enum class BossActions
{
    Idle,
    Taunt,
    Chase,
    Combo1, // ShieldStrikes
    Combo2,
    Combo3,
    Prepare, // OverheadStrike
    Jump,
    Dash,
    Land,
    Attack,
    Recover,
    Waiting,
    Start, // Mirage
    Charge,
    End,
    WaterSpouts,
    Load, // ShieldBlast
    PreShoot,
    Shoot,
};

class Boss : public Character
{
  public:
    Boss(GameObject* parent);
    ~Boss() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    GameObject* GetCloseArea() const { return closeArea; }
    int GetCloseAreaDamage() const { return closeAreaDamage; }

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void HandleState(float deltaTime) override;
    void UpdateTimers(float deltaTime) override;
    void ChooseNextState();
    void ChooseNextStateFirstPhase();
    void ChooseNextStateSecondPhase();
    void ChooseNextStateThirdPhase();

    void Idle();
    void Taunt(float deltaTime);
    void ShieldStrikes(float deltaTime);

    void OverheadStrike(float deltaTime);
    void StartDash();
    void Dash(float deltaTime);
    void StartJump();
    void Jump(float deltaTime);
    void StartFall();
    void Fall(float deltaTime);
    void DamageAreaLogic();

    BossDistance CheckDistance() const;
    void StopAttacking();

    void Mirage();
    void ChangePhase();
    void ResetValues(bool isForMirage = false);

    void ShieldBlast(float deltaTime);

    void WaterSpouts();

    void SetState(BossStates newState);
    BossStates ChooseAlternativeState() const;

    const std::vector<BossStates>& GetAvailableStates() const;
    const char* GetStateName() const;
    const char* GetActionName() const;

  private:
    AIAgentComponent* agentAI = nullptr;
    BossStates currentState   = BossStates::Idle;
    BossActions currentAction = BossActions::Idle;

    int phase                 = 1;
    int phase1 = 40, phase2 = 20, phase3 = 0;
    std::array<int, 3> phaseSwap = {phase1, phase2, phase3};
    bool stateEnter              = true;
    bool doIdle                  = false;
    bool doTaunt                 = false;
    bool actionTriggerDone       = false;

    int shieldStrikeLastAction   = 0;

    // Dash
    bool isDashing               = false;
    float dashSpeed              = 0.0f;
    float dashTimeRemaining      = 0.0f;
    float dashDistance           = 0.0f;
    float3 dashDirection         = float3::zero;
    float3 dashStartPosLocal     = float3::zero;

    // Jump
    bool isJumping               = false;
    float jumpSpeed              = 0.0f;
    float jumpTimeRemaining      = 0.0f;
    float3 jumpStartPosLocal     = float3::zero;

    // Fall
    bool isFalling               = false;
    float fallSpeed              = 0.0f;
    float fallTimeRemaining      = 0.0f;
    float3 fallStartPosLocal     = float3::zero;

    std::mt19937 rng;
    std::uniform_int_distribution<int> uniformDist;

    // Colliders
    std::string shieldName                           = "";
    std::string closeAreaName                        = "";
    GameObject* closeArea                            = nullptr;
    std::string bigAreaName                          = "";
    GameObject* bigArea                              = nullptr;
    float bigAreaHitboxDelay                         = 1.3f;
    CapsuleColliderComponent* blastArea              = nullptr;
    float blastHitboxDelay                           = 1.3f;

    // VFX
    std::string overheadPrepareVFXName               = "";
    ShaderScriptComponent* runesScript               = nullptr;
    MovingUVTransparent* runesUV                     = nullptr;
    ShaderScriptComponent* runesLightsScript         = nullptr;
    MovingUVTransparent* runesLightsUV               = nullptr;

    std::string overheadDashVFXName                  = "";
    MeshComponent* dashGroundMesh                    = nullptr;
    MeshComponent* dashEnergyMesh                    = nullptr;
    ShaderScriptComponent* dashLightsShieldScript    = nullptr;
    MovingUVTransparent* dashLightsShieldUV          = nullptr;
    ShaderScriptComponent* dashShieldExpansionScript = nullptr;
    MovingUVTransparent* dashShieldExpansionUV       = nullptr;

    std::string overheadAttackVFXName                = "";
    ShaderScriptComponent* attackLightingsScript     = nullptr;
    MovingUVTransparent* attackLightingsUV           = nullptr;
    ShaderScriptComponent* attackEnergyScript        = nullptr;
    MovingUVTransparent* attackEnergyUV              = nullptr;

    ShaderScriptComponent* attackExplosionScript     = nullptr;
    MovingUVTransparent* attackExplosionUV           = nullptr;
    ShaderScriptComponent* bigExpansionScript        = nullptr;
    MovingUVTransparent* bigExpansionUV              = nullptr;
    ShaderScriptComponent* smallExpansionScript      = nullptr;
    MovingUVTransparent* smallExpansionUV            = nullptr;

    std::string shieldBlastVFXName                   = "";
    MeshComponent* blastPreHitMesh                   = nullptr;
    ShaderScriptComponent* blastHitScript            = nullptr;
    MovingUVTransparent* blastHitUV                  = nullptr;
    ShaderScriptComponent* blastBlackLightsScript    = nullptr;
    MovingUVTransparent* blastBlackLightsUV          = nullptr;
    ShaderScriptComponent* blastSphereEnergyScript   = nullptr;
    MovingUVTransparent* blastSphereEnergyUV         = nullptr;
    ShaderScriptComponent* blastBlackExpansionScript = nullptr;
    MovingUVTransparent* blastBlackExpansionUV       = nullptr;

    // Particle
    std::string atomParticleName                     = "";
    ParticleSystemComponent* atomParticle            = nullptr;
    std::string smokeParticleName                    = "";
    ParticleSystemComponent* smokeParticle           = nullptr;
    std::string chargeShieldParticleName             = "";
    ParticleSystemComponent* chargeShieldParticle    = nullptr;

    // Inspector values
    int closeAreaDamage                              = 2;
    float dashDuration                               = 0.5f;
    float heightJump                                 = 4.0f;
    float jumpDuration                               = 0.2f;
    float fallDuration                               = 0.2f;

    // Health UI
    ImageComponent* healthImageComponent             = nullptr;
    UID healthBarImage;

    // Mirage
    const int mirage1 = 50, mirage2 = 30, mirage3 = 10;
    const std::array<int, 3> mirageActivation  = {mirage1, mirage2, mirage3};
    BossMirage* bossMirageScript               = nullptr;
    bool mirageActivated                       = false;

    int repeatedState                          = 0;
    const int maxRepeats                       = 2;
    // WaterSpout
    Spouts* waterSpout = nullptr;
    std::string spoutName = "";

    const std::vector<BossStates> phase1States = {BossStates::ShieldStrikes, BossStates::OverheadStrike};
    const std::vector<BossStates> phase2States = {BossStates::ShieldStrikes, BossStates::ShieldBlast};
    const std::vector<BossStates> phase3States = {
        BossStates::ShieldStrikes, BossStates::ShieldBlast, BossStates::OverheadStrike
    };
};
