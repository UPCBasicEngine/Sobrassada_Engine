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
    HURT
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
        GLOG("Enemy out. Total unique enemies colliding: %zu",  enemiesCont);
    }
    void OnEnemyHit();
    void OnEnemyDefeated();

    void ActivateAbility(std::string abilityName);
    void OnArrowHit();
  
    void StartCurse();
    
    void ExportState(PlayerState& playerState) const;
    void ApplySavedState(const PlayerState& playerState);
    
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

    bool IsBlockedAhead(const GameObject* ownerGO, const float3& desiredMoveDirection, float lookAheadDistance, float skinWidth);

    void SetPosition(const float3& position);
    const std::string GetLogicStateName();

    

  private:
    CharacterStates state                = CharacterStates::IDLE;

    int enemiesCont                   = 0;
    std::string cameraName             = "Camera Pivot";
    GameObject* cameraObject           = nullptr;
    CameraMovement* camera             = nullptr;

    std::string spearName                = "SpearProjectile";
    std::string spearNameMesh             = "WP_Spear_Cu_Chu";
    Projectile* spear                    = nullptr;
    GameObject* spearCharacter            = nullptr;

    float defaultSpeed                   = 7.0f;
    float inputBuffer                    = 0.5f;

    std::string healthBarName            = "HealthBarFill";
    BarFill* healthBar                   = nullptr;

    // Dash
    std::string dashIconName             = "DashCooldown";
    AbilityIconFill* dashIcon            = nullptr;
    float3 lastDashStartPos              = float3::zero;
    bool isDashing                       = false;
    bool wasDashing                      = false;
    float dashCooldown                   = 2.0f;
    float dashTimer                      = 0.0f;
    bool desiredDash                     = false;
    float dashBufferTimer                = 0.0f;
    bool dashUnlocked                    = false;

    // Basic attack
    std::string meleeVfxName             = "SpearVFX";
    std::string meleeTrailName           = "SpearMeleeTrail";
    std::string attackVfxHorizontal1Name = "AttackVfxH1";
    std::string attackVfxVertical1Name   = "AttackVfxV1";
    std::string attackVfxHorizontal2Name = "AttackVfxH2";
    std::string attackVfxVertical2Name   = "AttackVfxV2";
    std::string attackVfxHorizontal3Name = "AttackVfxH3";
    std::string attackVfxVertical3Name   = "AttackVfxV3";
    GameObject* meleeVfxObject           = nullptr;
    GameObject* meleeTrailObject         = nullptr;
    GameObject* attackVfxHorizontal1     = nullptr;
    GameObject* attackVfxVertical1       = nullptr;
    GameObject* attackVfxHorizontal2     = nullptr;
    GameObject* attackVfxVertical2       = nullptr;
    GameObject* attackVfxHorizontal3     = nullptr;
    GameObject* attackVfxVertical3       = nullptr;
    bool desiredAttack                   = false;
    float attackBufferTimer              = 0.0f;
    int comboCounter                     = -1;
    float comboBufferTimer               = 0.0f;
    float meleeVfxDelay                  = 0.1f;

    //Arrow Hit VFX 
    GameObject* arrowHitVfxObject         = nullptr;
    std::string arrowHitVfxName           = "";
    float arrowHitVfxDuration             = 0.2f;
    float arrowHitVfxTimer                = 0.0f;
    bool arrowVfxIsActive                 = false;

  
    // Charged attack
    std::string chargedAttackName        = "Charged";
    GameObject* chargedAttackCollider    = nullptr;
    bool isChargingAttack                = false;
    float chargeTimer                    = 0.0f;
    float chargeDuration                 = 1.0f;
    bool desiredChargedAttack            = false;
    float chargedAttackTimer             = 0.0f;
    float chargedAttackHitboxDelay       = 0.0f;
    float chargedAttackHitboxDuration    = 0.0f;
    int chargedAttackDamage              = 0;
    float attackPressTimer               = 0.0f;
    float chargeThreshold                = 0.2f;

    bool desiredAim                      = false;
    float throwTimer                     = 0.0f;
    float throwCooldown                  = 1.0f;
    bool resetWeapon                     = false;

    int reservedHealth                   = 0;
    float deathTimer                     = 0.5f;
    float aimTimer                       = 0.0f;

    std::string aimShadowName            = "AimShadow";
    GameObject* aimShadowObject          = nullptr;

    // Ultimate
    std::string ultimateIconName         = "UltimateCooldown";
    AbilityIconFill* ultimateIcon        = nullptr;
    std::string ultimateName             = "ultimate_attack";
    std::string ultimateGlowName         = "ulti_glow";
    std::string ultimateBlurName         = "ultimate_mesh_blur";
    std::string ultimateBrustName        = "ultimate_mesh_brust";
    std::string ultimateCrackName       = "ultimate_mesh_crack2";
    std::string ultimateHaloName         = "mesh_halo";
    std::string ultimateSmokeName        = "mesh_outer_smoke";
    std::string ultimateSphereName       = "mesh_sphere_glow";
    std::string ultimateWarningName      = "ultimate_mesh_warning";
    std::string ultimateSpikesName       = "ult_spike";
    GameObject* ultimateObject           = nullptr;
    GameObject* ultimateGlow             = nullptr;
    GameObject* ultimateBlur             = nullptr;
    GameObject* ultimateBrust            = nullptr;
    GameObject* ultimateCrack           = nullptr;
    GameObject* ultimateHalo             = nullptr;
    GameObject* ultimateSmoke            = nullptr;
    GameObject* ultimateSphere           = nullptr;
    GameObject* ultimateWarning          = nullptr;
    GameObject* ultimateSpikes           = nullptr;
    bool desiredUltimate                 = false;
    int ultimateDamage                   = 0;
    float ultimateTimer                  = 0.0f;
    float ultimateCd                     = 0.0f;
    float ultimateCdTimer                = 0.0f;
    float ultimateBufferTimer            = 0.0f;
    float ultimateHitboxDelay            = 0.0f;
    float ultimateHitboxDuration         = 0.0f;
    float ultimateAnimationDelay         = 0.0f;
    bool ultimateUnlocked                = false;

    // Riastrad
    std::string riastradBarName          = "BarFill";
    BarFill* riastradBar                 = nullptr;
    int riastradMeter                    = 0;
    bool isRiastrad                      = false;
    bool desiredTransform                = false;
    float transformBufferTimer           = 0.0f;
    float transformTimer                 = 0.0f;
    float transformVfxDelay              = 0.35f;
    float riastradTimer                  = 0.0f;
    float riastradDuration               = 5.0f;
    float riastradMovementSpeed          = 12.0f;
    float riastradAnimationsSpeedRatio   = 1.5f;
    int riastradOnDamageTaken            = 2;
    int riastradOnHit                    = 5;
    int riastradOnEnemyDeath             = 5;
    std::string riastradVfxName          = "riastrad_all";
    std::string riastradBurstName        = "mesh_brust";
    std::string riastradBlurName         = "mesh_blur";
    std::string riastradHaloName         = "mesh_halo";
    std::string riastradSphereName       = "mesh_sphere_glow";
    std::string riastradCrackName        = "mesh_crack";
    std::string riastradWaringName       = "mesh_warning";
    std::string riastradSmoke1Name       = "mesh_smoke_a";
    std::string riastradSmoke2Name       = "mesh_smoke_b";
    std::string riastradSmoke3Name       = "mesh_smoke_c";
    std::string riastradStarsName        = "mesh_stars";
    GameObject* riastradVfx              = nullptr;
    GameObject* riastradBurst            = nullptr;
    GameObject* riastradBlur             = nullptr;
    GameObject* riastradHalo             = nullptr;
    GameObject* riastradSphere           = nullptr;
    GameObject* riastradCrack            = nullptr;
    GameObject* riastradWaring           = nullptr;
    GameObject* riastradSmoke1           = nullptr;
    GameObject* riastradSmoke2           = nullptr;
    GameObject* riastradSmoke3           = nullptr;
    GameObject* riastradStars            = nullptr;

    float3 spawnPos                      = float3::zero;
    AudioSourceComponent* audio          = nullptr;

    float3 camFront                      = float3::zero;
    float3 camRight                      = float3::zero;

    bool godMode                         = false;
    float idleTimer                      = 0.0f;
    float runTimer                       = 0.0f;
    float stepTime                       = 0.367f;

    int mushrooms                        = 0;
    int mushroomHeal                     = 2;
    bool desiredTakeMushroom             = false;
    float takeMushroomCdTimer            = 0.0f;
    float takeMushroomCd                 = 0.0f;

    std::string dashTrailName            = "DashTrail";
    GameObject* dashTrail                = nullptr;
    std::string dashDecalName            = "DashDecal";
    GameObject* dashDecal                = nullptr;

    float dashDecalTimer                 = 5.0f;
    float dashDecalBufferTimer           = 0.0f;

    // Heal
    bool isHealing                       = false;
    std::string healVfxName              = "HealVfx";
    std::string healParticlesName        = "HealParticles";
    std::string healKnockbackName        = "Heal Knockback";
    GameObject* healVfx                  = nullptr;
    GameObject* healParticles            = nullptr;
    GameObject* healKnockback            = nullptr;
    float healTimer                      = 0.0f;
    float healKnockbackDelay             = 0.0f;

    std::string damageMaskName         = "DamageMask";
    DamageMask* damageMask             = nullptr;

    // Curse
    bool isCursed                        = false;
    float curseSpeed                     = 4.0f;
    float curseDuration                  = 5.0f;
    float curseTimer                     = 0.0f;
    UID playerMaterial                   = 0;
};

extern CharacterControllerComponent* character;
extern CuChulainn* playerScript;