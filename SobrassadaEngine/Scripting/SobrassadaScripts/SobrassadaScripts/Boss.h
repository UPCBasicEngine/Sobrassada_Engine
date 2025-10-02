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
class Spouts;
class AttackVfxSpritesheet;
class AudioSourceComponent;
class AnimationComponent;

enum class BossDistance
{
    None,
    Close,   // attack range (3m)
    Near,    // 6m
    Medium,  // 9m
    Distant, // 12m
    Far,     // 15m
    Farther, // ...
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
    Restart,
};

enum class BossActions
{
    None,
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
    WaterSpoutCharge, // WaterSpout
    WaterSpoutHit,
    Load, // ShieldBlast
    PreShoot,
    Shoot,
    Return,
};

class Boss : public Character
{
  public:
    Boss(GameObject* parent);
    ~Boss() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void OnPlayerExitLocation() override;
    void OnPlayerEnterLocation() override;

    void PlayHighlightSequence() override;

    void DisableBlastArea();

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

    void Idle(float deltaTime);
    void Taunt(float deltaTime);
    void Run();
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

    void Restart(float deltaTime);

    const std::vector<BossStates>& GetAvailableStates() const;
    const char* GetStateName() const;
    const char* GetActionName() const;

  private:
    AIAgentComponent* agentAI   = nullptr;
    AudioSourceComponent* audio = nullptr;
    BossStates currentState     = BossStates::Idle;
    BossActions currentAction   = BossActions::Idle;

    bool waiting                = true;
    bool restart                = false;
    float runTimer              = 0.0f;

    bool highlightActivated     = false;
    float highlightTimer        = 0.0f;
    bool playedHighlight        = false;

    int phase                   = 1;
    int phase2 = 40, phase3 = 20;
    std::array<std::reference_wrapper<int>, 2> phaseSwap = {phase2, phase3};
    bool stateEnter                                      = true;
    bool doIdle                                          = false;
    bool doTaunt                                         = false;
    bool actionTriggerDone                               = false;

    // ShieldStrikes
    std::string shieldName                               = "";
    int shieldStrikeLastAction                           = 0;
    float chaseTimer                                     = 0.0f;
    bool audioPlayed                                     = false;

    // OverheadStrike
    std::string closeAreaName                            = "";
    GameObject* closeArea                                = nullptr;
    std::string bigAreaName                              = "";
    GameObject* bigArea                                  = nullptr;
    float bigAreaHitboxDelay                             = 1.3f;

    // Dash
    bool isDashing                                       = false;
    float dashSpeed                                      = 0.0f;
    float dashTimeRemaining                              = 0.0f;
    float dashDistance                                   = 0.0f;
    float3 dashDirection                                 = float3::zero;
    float3 dashStartPosLocal                             = float3::zero;

    // Jump
    bool isJumping                                       = false;
    float jumpSpeed                                      = 0.0f;
    float jumpTimeRemaining                              = 0.0f;
    float3 jumpStartPosLocal                             = float3::zero;

    // Fall
    bool isFalling                                       = false;
    float fallSpeed                                      = 0.0f;
    float fallTimeRemaining                              = 0.0f;
    float3 fallStartPosLocal                             = float3::zero;

    std::mt19937 rng;
    std::uniform_int_distribution<int> uniformDist;
    std::uniform_int_distribution<int> uniformSteps;

    // VFX
    std::string emessiveVFXName                      = "";
    MeshComponent* emessiveVFXMesh                   = nullptr;

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
    ShaderScriptComponent* blastPreSpriteScript      = nullptr;
    AttackVfxSpritesheet* blastPreSpritesheet        = nullptr;
    ShaderScriptComponent* blastSpriteScript         = nullptr;
    AttackVfxSpritesheet* blastSpritesheet           = nullptr;
    ShaderScriptComponent* blastSpriteScript2        = nullptr;
    AttackVfxSpritesheet* blastSpritesheet2          = nullptr;
    ShaderScriptComponent* blastEnergySpriteScript   = nullptr;
    AttackVfxSpritesheet* blastEnergySpritesheet     = nullptr;

    std::string invulnerableVFXName                  = "";
    AnimationComponent* invulnerableAnimation        = nullptr;
    ShaderScriptComponent* invulnerableSpriteScript  = nullptr;
    AttackVfxSpritesheet* invulnerableSpritesheet    = nullptr;
    ShaderScriptComponent* invulnerableBarrierScript = nullptr;
    MovingUVTransparent* invulnerableBarrierUV       = nullptr;
    ShaderScriptComponent* invulnerableAuraScript    = nullptr;
    MovingUVTransparent* invulnerableAuraUV          = nullptr;

    // Particle
    std::string atomParticleName                     = "";
    ParticleSystemComponent* atomParticle            = nullptr;
    std::string smokeParticleName                    = "";
    ParticleSystemComponent* smokeParticle           = nullptr;
    std::string chargeShieldParticleName             = "";
    ParticleSystemComponent* chargeShieldParticle    = nullptr;
    std::string energyBlastParticleName              = "";
    ParticleSystemComponent* energyBlastParticle1    = nullptr;
    ParticleSystemComponent* energyBlastParticle2    = nullptr;
    ParticleSystemComponent* energyBlastParticle3    = nullptr;
    ParticleSystemComponent* energyBlastParticle4    = nullptr;

    // Inspector values
    int closeAreaDamage                              = 3;
    float dashDuration                               = 0.3f;
    float heightJump                                 = 4.0f;
    float jumpDuration                               = 0.2f;
    float fallDuration                               = 0.2f;
    float highlightDelay                             = 8.0f;
    float chaseTimeLimit                             = 8.0f;
    float blastAreaDisabledLimit                     = 0.5f;
    float stepTime                                   = 0.5f;

    // Health UI
    ImageComponent* healthImageComponent             = nullptr;
    UID healthBarImage;

    // ShieldBlast
    std::string blastAreaName = "";
    GameObject* blastArea     = nullptr;
    float blastHitboxDelay    = 1.3f;
    bool blastHit             = false;
    float blastHitTimer       = 0.0f;

    // Mirage
    int mirage1 = 47, mirage2 = 30, mirage3 = 10;
    std::array<std::reference_wrapper<int>, 3> mirageActivation = {mirage1, mirage2, mirage3};
    BossMirage* bossMirageScript                                = nullptr;
    bool mirageActivated                                        = false;

    // WaterSpout
    std::vector<Spouts*> waterSpouts;
    std::string spoutName                      = "";

    // Alternate mechanic
    int repeatedState                          = 0;
    const int maxRepeats                       = 2;
    const std::vector<BossStates> phase1States = {BossStates::ShieldStrikes, BossStates::OverheadStrike};
    const std::vector<BossStates> phase2States = {
        BossStates::ShieldStrikes, BossStates::ShieldBlast, BossStates::WaterSpouts
    };
    const std::vector<BossStates> phase3States = {
        BossStates::ShieldStrikes, BossStates::ShieldBlast, BossStates::OverheadStrike, BossStates::WaterSpouts
    };
};
