#pragma once

#include "Character.h"

class GameObject;
class CharacterControllerComponent;
class CameraMovement;
class Projectile;

enum class CharacterStates
{
    NONE,
    IDLE,
    RUN,
    DASH,
    BASIC_ATTACK,
    AIM
};

class CuChulainn : public Character
{
  public:
    CuChulainn(GameObject* parent);
    virtual ~CuChulainn() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void SetSpawnPosition(const float3& newPos) { spawnPos = newPos; }

  private:
    void OnDeath() override;
    void OnDamageTaken(int amount) override;
    void OnHealed(int amount) override;
    void PerformAttack() override;
    void HandleState(float deltaTime) override;

    bool CanDash();
    bool CanAttack(float deltaTime) override;
    bool CanAim() const;
    void GetInputs();
    void UpdateTimers(float deltaTime);
    void LookAtMouse();

    void ThrowSpear();
    void Attack(float time) override;
    void Dash();
    void Aim();
    void Move();
    void Respawn();

  private:
    std::string cameraName  = "";
    CameraMovement* camera  = nullptr;

    std::string spearName   = "";
    Projectile* spear       = nullptr;

    bool isDashing          = false;
    float dashCooldown      = 2.0f;
    float dashTimer         = 0.0f;
    bool desiredDash        = false;
    float dashBufferTimer   = 0.0f;
    float dashBuffer        = 0.5f;

    bool desiredAttack      = false;
    float attackTimer       = 0.0f;
    float attackBufferTimer = 0.0f;
    float attackBuffer      = 0.5f;

    bool desiredAim         = false;
    float throwTimer        = 0.0f;
    float throwCooldown     = 1.0f;
    bool resetWeapon        = false;

    CharacterStates state   = CharacterStates::IDLE;
    float3 spawnPos         = float3::zero;
};

extern CharacterControllerComponent* character;
