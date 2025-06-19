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
    CHARGED_ATTACK
};

class CuChulainn : public Character
{
  public:
    CuChulainn(GameObject* parent);
    virtual ~CuChulainn() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void SetSpawnPosition(const float3& newPos) { spawnPos = newPos; }
    void SetDeath(bool death) { isDead = death; }
    void SetHealth(int health) { reservedHealth = health; }
    void Respawn();
    void UpdateHealthBarUI();
    bool GetIsInvulnerable() { return isInvulnerable; }
    void SetInvulnearble(bool invulnerable) { isInvulnerable = invulnerable; }
    CharacterStates GetState() const { return state; }
    int GetUltimateDamage() const { return ultimateDamage; }
    int GetChargedAttackDamage() const { return chargedAttackDamage; }

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void OnHealed(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;
    void TakeDamage(int amount) override;

    bool CanDash() const;
    bool CanAttack() const;
    bool CanUltimate() const;
    bool CanAim() const;
    bool CanChargeAttack() const;
    void GetInputs();
    void UpdateTimers(float deltaTime);
    void LookAtMouse();
    void LookAtRightStick();
    void LookAtLeftStick();
    void CheckIsFalling();

    void ThrowSpear();
    void Attack(float deltaTime) override;
    void UltimateAttack();
    void Dash();
    void Aim(float deltaTime);
    void Move();
    void ChargeAttack();

    void SetPosition(const float3& position);
    const std::string GetLogicStateName();

  private:
    CharacterStates state             = CharacterStates::IDLE;

    std::string cameraName            = "";
    GameObject* cameraObject          = nullptr;
    CameraMovement* camera            = nullptr;

    std::string spearName             = "";
    Projectile* spear                 = nullptr;

    float inputBuffer                 = 0.5f;

    float3 lastDashStartPos           = float3::zero;
    bool isDashing                    = false;
    float dashCooldown                = 2.0f;
    float dashTimer                   = 0.0f;
    bool desiredDash                  = false;
    float dashBufferTimer             = 0.0f;

    bool desiredAttack                = false;
    float attackBufferTimer           = 0.0f;
    int comboCounter                  = -1;
    float comboBufferTimer            = 0.0f;

    std::string chargedAttackName     = "";
    GameObject* chargedAttackCollider = nullptr;
    bool isChargingAttack             = false;
    float chargeTimer                 = 0.0f;
    float chargeDuration              = 1.0f;
    bool desiredChargedAttack         = false;
    float chargedAttackTimer          = 0.0f;
    float chargedAttackHitboxDelay    = 0.0f;
    float chargedAttackHitboxDuration = 0.0f;
    int chargedAttackDamage           = 0;
    float attackPressTimer            = 0.0f;

    bool desiredAim                   = false;
    float throwTimer                  = 0.0f;
    float throwCooldown               = 1.0f;
    bool resetWeapon                  = false;

    int reservedHealth                = 0;
    float deathTimer                  = 0.5f;
    float aimTimer                    = 0.0f;

    std::string ultimateName          = "";
    GameObject* ultimateObject        = nullptr;
    bool desiredUltimate              = false;
    int ultimateDamage                = 0;
    float ultimateTimer               = 0.0f;
    float ultimateCd                  = 0.0f;
    float ultimateCdTimer             = 0.0f;
    float ultimateBufferTimer         = 0.0f;
    float ultimateHitboxDelay         = 0.0f;
    float ultimateHitboxDuration      = 0.0f;

    float3 spawnPos                   = float3::zero;
    AudioSourceComponent* audio       = nullptr;

    float3 camFront                   = float3::zero;
    float3 camRight                   = float3::zero;

    std::vector<UID> healthBarTextures;
    ImageComponent* healthImageComponent = nullptr;

    bool godMode                         = false;
};

extern CharacterControllerComponent* character;