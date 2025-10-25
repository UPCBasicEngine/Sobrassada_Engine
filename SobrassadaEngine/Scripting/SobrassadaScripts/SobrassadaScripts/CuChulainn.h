#pragma once

#include "Character.h"
#include "Globals.h"
#include "SavePlayerData.h"

class GameObject;
class CharacterControllerComponent;
class CameraMovement;
class Projectile;
class AudioSourceComponent;
class ImageComponent;
class BarFill;
class AbilityIconFill;
class DamageMask;
class AttackVfxSpritesheet;
class ShaderScriptComponent;
class UISpritesheet;

enum class CharacterStates
{
    NONE,
    IDLE,
    RUN,
    DASH,
    BASIC_ATTACK,
    AIM,
    RESPAWN,
    DEATH,
    FALL,
    ULTIMATE,
    CHARGING,
    CHARGED_ATTACK,
    TAKE_MUSHROOM,
    HEAL,
    TRANSFORM,
    HURT,
};

constexpr const char* BlockerGOTags[] = {"MagicBarrier"};

class CuChulainn : public Character
{
  public:
    CuChulainn(GameObject* parent);
    virtual ~CuChulainn() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;
    void OnDestroy() override;

    void Respawn();
    bool TakeMushroom();
    bool CanTakeMushroom() const;

    bool GetIsInvulnerable() { return isInvulnerable; }
    CharacterStates GetState() const { return state; }
    int GetUltimateDamage() const { return ultimateDamage; }
    int GetChargedAttackDamage() const { return chargedAttackDamage; }
    int GetMushrooms() const { return mushrooms; }
    bool GetDesiredTakeMushroom() const { return desiredTakeMushroom; }
    int GetRiastradMeter() const { return riastradMeter; }
    bool IsDashUnlocked() const { return dashUnlocked; }
    bool IsUltimateUnlocked() const { return ultimateUnlocked; }
    int GetEnemiesCount() const { return enemiesCont; }
    bool HasblockingTag(GameObject* go);

    void SetSpawnPosition(const float3& newPos) { spawnPos = newPos; }
    void SetDeath(bool death) { isDead = death; }
    void SetHealth(int health) { reservedHealth = health; }
    void SetInvulnearble(bool invulnerable) { isInvulnerable = invulnerable; }
    void AddEnemy() { enemiesCont++; }
    void RemoveEnemy()
    {
        if (enemiesCont != 0) enemiesCont--;
        GLOG("Enemy out. Total unique enemies colliding: %zu", enemiesCont);
    }
    void ResetState();

    void OnObjectDestroyed();
    void OnEnemyHit();
    void OnEnemyDefeated();
    void ActivateArrowMark();
    void SetArrowMark(float3 posArrow);

    void ActivateAbility(std::string abilityName);
    void OnArrowHit();

    void StartCurse();

    void ExportState(PlayerState& playerState) const;
    void ApplySavedState(const PlayerState& playerState);
    bool ConsumeJustDied();
    bool IsGameOverCondition() const;

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void OnHealed(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void TakeDamage(int amount) override;
    void UpdateTimers(float deltaTime) override;
    void Attack(float deltaTime) override;
    // void OnCollisionEnter(GameObject* otherObject, float3 collisionNormal, ColliderLayer layer) override;

    bool CanHeal() const;
    bool CanDash() const;
    bool CanAttack() const;
    bool CanUltimate() const;
    bool CanAim() const;
    bool CanChargeAttack() const;
    bool CanTransform() const;
    void GetInputs();

    void LookAtMouse();
    void LookAtRightStick();
    void LookAtLeftStick();
    void CheckIsFalling();

    void EnableRiastradVfx();
    void UseMushroom();
    void ThrowSpear();
    void UltimateAttack();
    void UpdateUltimateVfx();
    void Dash();
    void Aim(float deltaTime);
    void Move();
    void ChargeAttack();
    void ToggleRiastrad();
    void AddRiastrad(int amount);
    void EndCurse();

    bool IsBlockedAhead(
        const GameObject* ownerGO, const float3& desiredMoveDirection, float lookAheadDistance, float skinWidth
    );

    void SetPosition(const float3& position);
    const std::string GetLogicStateName();

  private:
    CharacterStates state                          = CharacterStates::IDLE;

    int enemiesCont                                = 0;
    std::string cameraName                         = "Camera Pivot";
    GameObject* cameraObject                       = nullptr;
    CameraMovement* camera                         = nullptr;

    std::string spearName                          = "SpearProjectile";
    std::string spearNameMesh                      = "WP_Spear_Cu_Chu";
    Projectile* spear                              = nullptr;
    GameObject* spearCharacter                     = nullptr;

    float defaultSpeed                             = 7.0f;
    float inputBuffer                              = 0.5f;

    std::string healthBarName                      = "HealthBarFill";
    BarFill* healthBar                             = nullptr;

    bool controlsLocked                            = false;

    // Dash
    std::string dashIconName                       = "DashCooldown";
    std::string dashSmokeName1                     = "DashSmoke1";
    std::string dashSmokeName2                     = "DashSmoke2";
    AbilityIconFill* dashIcon                      = nullptr;
    ShaderScriptComponent* dashSmoke1              = nullptr;
    ShaderScriptComponent* dashSmoke2              = nullptr;
    float3 lastDashStartPos                        = float3::zero;
    bool isDashing                                 = false;
    bool wasDashing                                = false;
    float dashCooldown                             = 0.5f;
    float dashTimer                                = 0.0f;
    bool desiredDash                               = false;
    float dashBufferTimer                          = 0.0f;
    bool dashUnlocked                              = false;

    // Basic attack
    std::string meleeTrailName                     = "SpearMeleeTrail";
    std::string attackVfxHorizontal1Name           = "AttackVfxH1";
    std::string attackVfxVertical1Name             = "AttackVfxV1";
    std::string attackVfxHorizontal2Name           = "AttackVfxH2";
    std::string attackVfxVertical2Name             = "AttackVfxV2";
    std::string attackVfxHorizontal3Name           = "AttackVfxH3";
    std::string attackVfxVertical3Name             = "AttackVfxV3";
    std::string attackVfxExplosionName             = "AttackExplosion";
    GameObject* meleeTrailObject                   = nullptr;
    ShaderScriptComponent* attackVfxHorizontal1    = nullptr;
    ShaderScriptComponent* attackVfxVertical1      = nullptr;
    ShaderScriptComponent* attackVfxHorizontal2    = nullptr;
    ShaderScriptComponent* attackVfxVertical2      = nullptr;
    ShaderScriptComponent* attackVfxHorizontal3    = nullptr;
    ShaderScriptComponent* attackVfxVertical3      = nullptr;
    ShaderScriptComponent* attackVfxExplosion      = nullptr;
    bool moveWithAttack                            = false;
    bool desiredAttack                             = false;
    float attackBufferTimer                        = 0.0f;
    float comboBufferTimer                         = 0.0f;
    float meleeVfxDelay                            = 0.1f;
    int comboCounter                               = -1;

    // Arrow Hit VFX
    GameObject* arrowHitVfxObject                  = nullptr;
    std::string arrowHitVfxName                    = "ArrowHit";
    float arrowHitVfxDuration                      = 0.2f;
    float arrowHitVfxTimer                         = 0.0f;
    bool arrowVfxIsActive                          = false;

     // Arrow Mark VFX
    GameObject* markVfxObject                      = nullptr;
    std::string markVfxName                        = "ArrowMark";
    float markVfxDuration                          = 0.5f;
    float markVfxTimer                             = 0.0f;
    bool markVfxIsActive                           = false;


    // Charged attack
    std::string chargedAttackName                  = "Charged";
    std::string chargeSpritesheetName1             = "chargeVFX1";
    std::string chargeSpritesheetName2             = "chargeVFX2";
    std::string chargeSpritesheetName3             = "chargeVFX3";
    std::string chargeAttackVfxName                = "ChargedVFX";
    GameObject* chargedAttackCollider              = nullptr;
    ShaderScriptComponent* chargeVfx1              = nullptr;
    ShaderScriptComponent* chargeVfx2              = nullptr;
    ShaderScriptComponent* chargeVfx3              = nullptr;
    ShaderScriptComponent* chargedAttackVfx        = nullptr;
    bool isChargingAttack                          = false;
    float chargeTimer                              = 0.0f;
    float chargeDuration                           = 1.0f;
    bool desiredChargedAttack                      = false;
    float chargedAttackTimer                       = 0.0f;
    float chargedAttackHitboxDelay                 = 0.3f;
    float chargedAttackHitboxDuration              = 0.1f;
    int chargedAttackDamage                        = 2;
    float attackPressTimer                         = 0.0f;
    float chargeThreshold                          = 0.2f;

    bool desiredAim                                = false;
    float throwTimer                               = 0.0f;
    float throwCooldown                            = 1.0f;
    bool resetWeapon                               = false;

    int reservedHealth                             = 0;
    float deathTimer                               = 0.5f;
    float aimTimer                                 = 0.0f;

    std::string aimShadowName                      = "AimShadow";
    GameObject* aimShadowObject                    = nullptr;

    // Ultimate
    std::string ultimateIconName                   = "UltimateCooldown";
    AbilityIconFill* ultimateIcon                  = nullptr;
    std::string ultimateName                       = "ultimate_attack";
    std::string ultimateGlowName                   = "ulti_glow";
    std::string ultimateBlurName                   = "ultimate_mesh_blur";
    std::string ultimateBrustName                  = "ultimate_mesh_brust";
    std::string ultimateCrackName                  = "ultimate_mesh_crack2";
    std::string ultimateWarningName                = "ultimate_mesh_warning";
    std::string ultimateSpikesName                 = "ult_spike";
    GameObject* ultimateObject                     = nullptr;
    GameObject* ultimateGlow                       = nullptr;
    GameObject* ultimateBlur                       = nullptr;
    GameObject* ultimateBrust                      = nullptr;
    GameObject* ultimateCrack                      = nullptr;
    GameObject* ultimateWarning                    = nullptr;
    GameObject* ultimateSpikes                     = nullptr;
    bool desiredUltimate                           = false;
    int ultimateDamage                             = 0;
    float ultimateTimer                            = 0.0f;
    float ultimateCd                               = 5.0f;
    float ultimateCdTimer                          = 0.0f;
    float ultimateBufferTimer                      = 0.0f;
    float ultimateHitboxDelay                      = 0.2f;
    float ultimateHitboxDuration                   = 0.4f;
    float ultimateAnimationDelay                   = 1.0f;
    bool ultimateUnlocked                          = false;
    bool playerAnimHeld                            = false;
    bool ultimateHoldEnabled                       = true;
    float ultimateResumeVfxTime                    = 1.5f;
    float vfxTimeUnscaledSec                       = 0.0f;
    bool ultimateSoundPlayed                       = false;
    float ultimateSpeed                            = 1.3f;

    // Riastrad
    std::string riastradBarName                    = "BarFill";
    std::string riastradEyeName                    = "RiastradEye";
    std::string riastradTriggersName               = "RiastradTriggers";
    std::string riastradKeyName                    = "RiastradKey";
    std::string riastradVfxBGName                  = "EyeBackgroundVFX";
    std::string riastradVfxFGName                  = "EyeForegroundVFX";
    std::string riastradFireUpName                 = "RiastradFireUp";
    std::string riastradFireDownName               = "RiastradFireDown";
    BarFill* riastradBar                           = nullptr;
    AbilityIconFill* riastradEye                   = nullptr;
    ShaderScriptComponent* riastradVfxBG           = nullptr;
    ShaderScriptComponent* riastradVfxFG           = nullptr;
    ShaderScriptComponent* riastradFireUp          = nullptr;
    ShaderScriptComponent* riastradFireDown        = nullptr;
    GameObject* riastradTriggers                   = nullptr;
    GameObject* riastradKey                        = nullptr;
    int riastradMeter                              = 0;
    bool isRiastrad                                = false;
    bool desiredTransform                          = false;
    float transformBufferTimer                     = 0.0f;
    float transformTimer                           = 0.0f;
    float transformVfxDelay                        = 0.3f;
    float riastradTimer                            = 0.0f;
    float riastradDuration                         = 5.0f;
    float riastradMovementSpeed                    = 12.0f;
    float riastradAnimationsSpeedRatio             = 1.5f;
    int riastradOnDamageTaken                      = 1;
    int riastradOnObjectHit                        = 1;
    int riastradOnEnemyHit                         = 3;
    int riastradOnEnemyDeath                       = 5;
    std::string riastradVfxName                    = "riastrad_attack";
    std::string riastradBlurName                   = "risastrad_mesh_blur";
    std::string riastradCrackName                  = "risastrad_mesh_crack";
    std::string riastradWarningName                = "risastrad_mesh_warning";
    std::string riastradStarsName                  = "risastrad_mesh_stars";
    std::string riastradSmokeName                  = "RiastradSmoke";
    std::string riastradGroundExplosionName        = "RiastradGroundExplosion";
    GameObject* riastradVfx                        = nullptr;
    GameObject* riastradBlur                       = nullptr;
    GameObject* riastradCrack                      = nullptr;
    GameObject* riastradWarning                    = nullptr;
    GameObject* riastradStars                      = nullptr;
    ShaderScriptComponent* riastradSmoke           = nullptr;
    ShaderScriptComponent* riastradGroundExplosion = nullptr;

    float3 spawnPos                                = float3::zero;
    AudioSourceComponent* audio                    = nullptr;

    float3 camFront                                = float3::zero;
    float3 camRight                                = float3::zero;

    bool godMode                                   = false;
    float idleTimer                                = 0.0f;
    float runTimer                                 = 0.0f;
    float stepTime                                 = 0.367f;
    bool justDied                                  = false;
    bool pendingGameOver                           = false;
    bool moveFromCollision                         = false;

    int mushrooms                                  = 0;
    int mushroomHeal                               = 2;
    bool desiredTakeMushroom                       = false;
    float takeMushroomCdTimer                      = 0.0f;
    float takeMushroomCd                           = 0.0f;

    std::string dashTrailName                      = "DashTrail";
    GameObject* dashTrail                          = nullptr;
    std::string dashDecalName                      = "DashDecal";
    GameObject* dashDecal                          = nullptr;

    float dashDecalTimer                           = 5.0f;
    float dashDecalBufferTimer                     = 0.0f;

    // Heal
    std::string hudMushroomName1                   = "HUDMushroom1";
    std::string hudMushroomName2                   = "HUDMushroom2";
    std::string hudMushroomName3                   = "HUDMushroom3";
    std::string hudMushroomUseName1                = "HUDMushroomUse1";
    std::string hudMushroomUseName2                = "HUDMushroomUse2";
    std::string hudMushroomUseName3                = "HUDMushroomUse3";
    std::string hudMushroomPickName1               = "HUDMushroomPick1";
    std::string hudMushroomPickName2               = "HUDMushroomPick2";
    std::string hudMushroomPickName3               = "HUDMushroomPick3";
    GameObject* hudMushrooms[3]                    = {nullptr};
    ShaderScriptComponent* hudMushroomsUse[3]      = {nullptr};
    ShaderScriptComponent* hudMushroomsPick[3]     = {nullptr};
    bool isHealing                                 = false;
    std::string healVfxName                        = "HealVfx";
    std::string healParticlesName                  = "HealParticles";
    std::string healKnockbackName                  = "Heal Knockback";
    GameObject* healVfx                            = nullptr;
    GameObject* healParticles                      = nullptr;
    GameObject* healKnockback                      = nullptr;
    float healTimer                                = 0.0f;
    float healKnockbackDelay                       = 1.0f;
    float enableMushroomTimer                      = 0.0f;
    bool mushroomToEnable                          = false;

    std::string damageMaskName                     = "DamageMask";
    std::string damageScratchName1                 = "DamageBR";
    std::string damageScratchName2                 = "DamageMid1";
    std::string damageScratchName3                 = "DamageMid2";
    std::string damageScratchName4                 = "DamageMid3";
    std::string damageScratchName5                 = "DamageTR";
    DamageMask* damageMask                         = nullptr;
    ShaderScriptComponent* damageScratch[5]        = {nullptr};

    // Curse
    bool isCursed                                  = false;
    float curseSpeed                               = 4.0f;
    float curseDuration                            = 5.0f;
    float curseTimer                               = 0.0f;
    UID playerMaterial                             = 0;

    float timeStopTimer                            = 0.0f;
    float hitTimeStopDuration                      = 0.05f;
    float deathTimeStopDuration                    = 0.1f;

    HashString defaultIdleName                     = HashString("CH_MC_Chu_AllAnimations_AN_MC_Idle");
    HashString riastradIdleName                    = HashString("CH_MC_Chu_AllAnimations_AN_IdleRiastrad");
    HashString riastradIdleName2                   = HashString("CH_MC_Chu_AllAnimations_AN_IdleRiastrad2");

    HashString defaultRunName                      = HashString("CH_MC_Chu_AllAnimations_AN_Run2");
    HashString riastradRunName                     = HashString("CH_MC_Chu_AllAnimations_AN_RunRiastrad");
    HashString curseRunName                        = HashString("CH_MC_Chu_AllAnimations_AN_MC_Chu_Walk_Pooka");
};

extern CharacterControllerComponent* character;
extern CuChulainn* playerScript;