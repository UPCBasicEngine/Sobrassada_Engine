#pragma once

#include "Script.h"

#include <vector>

class GameObject;
class CharacterControllerComponent;
class AnimationComponent;
class CapsuleColliderComponent;

enum class PlayerDistances
{
    Close,
    Medium,
    Far
};

enum class CharacterType
{
    None,
    CuChulainn,
    Soldier,
    Archer,
    Banshee
};

class Character : public Script
{
  public:
    Character(
        GameObject* parent, int maxHealth, int damage, float attackDuration, float cooldown, float range,
        float rangeAIAttack, float rangeAIChase, CharacterType type
    );
    virtual ~Character() noexcept override { parent = nullptr; };

    virtual bool Init() override;
    virtual void Update(float deltaTime) override;
    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

    void TakeDamage(int amount);

  protected:
    virtual void Attack(float deltaTime);
    virtual void UpdateTimers(float deltaTime);
    void Heal(int amount);
    PlayerDistances CheckDistanceWithPlayer() const;
    bool CheckDistanceWithPoint(const float3& point) const;

  private:
    virtual void HandleState(float deltaTime) {};
    virtual void OnDeath() {};
    virtual void OnDamageTaken(int amount) {}; // depending of amout damage taken do some sound or another for example
    virtual void OnHealed(int amount) {};
    virtual void PerformAttack() {};
    virtual void ShouldAttackTarget() {};
    virtual void Die();

  protected:
    AnimationComponent* animComponent           = nullptr;
    CapsuleColliderComponent* characterCollider = nullptr;
    std::string weaponName                      = "";
    GameObject* weapon                          = nullptr;
    CapsuleColliderComponent* weaponCollider    = nullptr;

    int maxHealth                               = 0;
    int currentHealth                           = 0;
    bool isInvulnerable                         = false;
    bool isDead                                 = false;
    float speed                                 = 0.0f;

    int attackDamage                            = 0;
    float attackDuration                        = 0.0f;
    float attackCooldown                        = 0.0f;
    float attackCdTimer                         = 0.0f;
    float range                                 = 0.0f;
    float attackTimer                           = 0.0f;
    bool isAttacking                            = false;
    float attackHitboxDelay                     = 0.0f;
    float attackHitboxDuration                  = 0.0f;

    float invulnerabilityTimer                  = -1.0f;
    const float invulnerableDuration            = 0.7f;

    CharacterType type                          = CharacterType::None;

    // AI
    float rangeAIChase                          = 0.0f;
    float rangeAIAttack                         = 0.0f;
    bool reachedPatrolPoint                     = false;
    float3 startPos                             = float3::zero;
};
