#pragma once

#include "Character.h"
#include "Globals.h"

class GameObject;
class CharacterControllerComponent;
class CameraMovement;
class Projectile;
class AudioSourceComponent;
class ImageComponent;

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
};

class CuChulainn : public Character
{
  public:
    CuChulainn(GameObject* parent);
    virtual ~CuChulainn() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;
    void OnDestroy() override;

    void Respawn();
    void UpdateHealthBarUI();
    void UpdateDashCooldownUI();
    void UpdateUltimateCooldownUI();
    bool TakeMushroom();
    bool CanTakeMushroom() const;

    bool GetIsInvulnerable() { return isInvulnerable; }
    CharacterStates GetState() const { return state; }
    int GetUltimateDamage() const { return ultimateDamage; }
    int GetChargedAttackDamage() const { return chargedAttackDamage; }
    int GetMushrooms() const { return mushrooms; }
    bool GetDesiredTakeMushroom() const { return desiredTakeMushroom; }

    void SetSpawnPosition(const float3& newPos) { spawnPos = newPos; }
    void SetDeath(bool death) { isDead = death; }
    void SetHealth(int health) { reservedHealth = health; }
    void SetInvulnearble(bool invulnerable) { isInvulnerable = invulnerable; }
    void OnEnemyHit();
    void OnEnemyDefeated();

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void OnHealed(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void TakeDamage(int amount) override;
    void UpdateTimers(float deltaTime) override;
    void Attack(float deltaTime) override;

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

    void UseMushroom();
    void ThrowSpear();
    void UltimateAttack();
    void Dash();
    void Aim(float deltaTime);
    void Move();
    void ChargeAttack();
    void ToggleRiastrad();
    void AddRiastrad(int amount);

    void SetPosition(const float3& position);
    const std::string GetLogicStateName();

  private:
    CharacterStates state              = CharacterStates::IDLE;

    std::string cameraName             = "";
    GameObject* cameraObject           = nullptr;
    CameraMovement* camera             = nullptr;

    std::string spearName              = "";
    Projectile* spear                  = nullptr;

    float defaultSpeed                 = 7.0f;
    float inputBuffer                  = 0.5f;

    float3 lastDashStartPos            = float3::zero;
    bool isDashing                     = false;
    bool wasDashing                    = false;
    float dashCooldown                 = 2.0f;
    float dashTimer                    = 0.0f;
    bool desiredDash                   = false;
    float dashBufferTimer              = 0.0f;

    std::string meleeVfxName           = "";
    std::string meleeTrailName         = "";
    GameObject* meleeVfxObject         = nullptr;
    GameObject* meleeTrailObject       = nullptr;
    bool desiredAttack                 = false;
    float attackBufferTimer            = 0.0f;
    int comboCounter                   = -1;
    float comboBufferTimer             = 0.0f;

    std::string chargedAttackName      = "";
    GameObject* chargedAttackCollider  = nullptr;
    bool isChargingAttack              = false;
    float chargeTimer                  = 0.0f;
    float chargeDuration               = 1.0f;
    bool desiredChargedAttack          = false;
    float chargedAttackTimer           = 0.0f;
    float chargedAttackHitboxDelay     = 0.0f;
    float chargedAttackHitboxDuration  = 0.0f;
    int chargedAttackDamage            = 0;
    float attackPressTimer             = 0.0f;

    bool desiredAim                    = false;
    float throwTimer                   = 0.0f;
    float throwCooldown                = 1.0f;
    bool resetWeapon                   = false;

    int reservedHealth                 = 0;
    float deathTimer                   = 0.5f;
    float aimTimer                     = 0.0f;

    std::string aimShadowName          = "";
    GameObject* aimShadowObject        = nullptr;

    std::string ultimateName           = "";
    GameObject* ultimateObject         = nullptr;
    bool desiredUltimate               = false;
    int ultimateDamage                 = 0;
    float ultimateTimer                = 0.0f;
    float ultimateCd                   = 0.0f;
    float ultimateCdTimer              = 0.0f;
    float ultimateBufferTimer          = 0.0f;
    float ultimateHitboxDelay          = 0.0f;
    float ultimateHitboxDuration       = 0.0f;
    float ultimateAnimationDelay       = 0.0f;

    GameObject* riastradBar            = nullptr;
    int riastradMeter                  = 0;
    bool isRiastrad                    = false;
    bool desiredTransform              = false;
    float transformBufferTimer         = 0.0f;
    float riastradTimer                = 0.0f;
    float riastradDuration             = 5.0f;
    float riastradMovementSpeed        = 12.0f;
    float riastradAnimationsSpeedRatio = 1.5f;
    int riastradOnDamageTaken          = 2;
    int riastradOnHit                  = 5;
    int riastradOnEnemyDeath           = 5;

    float3 spawnPos                    = float3::zero;
    AudioSourceComponent* audio        = nullptr;

    float3 camFront                    = float3::zero;
    float3 camRight                    = float3::zero;

    std::vector<UID> healthBarTextures;
    ImageComponent* healthImageComponent   = nullptr;
    ImageComponent* dashImageComponent     = nullptr;
    ImageComponent* ultimateImageComponent = nullptr;

    bool godMode                           = false;
    float idleTimer                        = 0.0f;
    float runTimer                         = 0.0f;
    float stepTime                         = 0.367f;

    int mushrooms                          = 0;
    int mushroomHeal                       = 2;
    bool desiredTakeMushroom               = false;
    float takeMushroomCdTimer              = 0.0f;
    float takeMushroomCd                   = 0.0f;

    std::string dashTrailName              = "";
    GameObject* dashTrail                  = nullptr;
    std::string dashDecalName              = "";
    GameObject* dashDecal                  = nullptr;

    float dashDecalTimer                   = 5.0f;
    float dashDecalBufferTimer             = 0.0f;

    bool isHealing                         = false;
    std::string healVisualName             = "";
    GameObject* healVisual                 = nullptr;

    // Images UIDs
    UID dashFillImage                      = 0;
    UID dashEmptyImage                     = 0;
    UID ultimateFillImage                  = 0;
    UID ultimateEmptyImage                 = 0;
};

extern CharacterControllerComponent* character;
extern CuChulainn* playerScript;