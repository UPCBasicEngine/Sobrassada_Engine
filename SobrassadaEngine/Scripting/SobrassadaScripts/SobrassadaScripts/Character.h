#pragma once

#include "Script.h"

#include <vector>

class GameObject;
class CharacterControllerComponent;
class AnimationComponent;
class CubeColliderComponent;
class CapsuleColliderComponent;

enum AIStates
{
    CLOSE = 0,
    MEDIUM,
    FAR_AWAY
};

enum class CharacterType
{
    None,
    CuChulainn,
    Soldier,
};

class Character : public Script
{
  public:
    Character(
        GameObject* parent, int maxHealth, int damage, float attackDuration, float cooldown, float range,
        float rangeAIAttack, float rangeAIChase, const float3& patrolPoint
    );
    virtual ~Character() noexcept override { parent = nullptr; };

    virtual bool Init() override;
    virtual void Update(float deltaTime) override;
    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

    void TakeDamage(int amount);

  protected:
    virtual void Attack(float deltaTime);
    void Heal(int amount);
    virtual bool CanAttack(float deltaTime);
    AIStates CheckDistanceWithPlayer() const;
    bool CheckDistanceWithPoint(const float3& point) const;

  private:
    virtual void HandleState(float deltaTime) {};
    virtual void OnDeath() {};
    virtual void OnDamageTaken(int amount) {}; // depending of amout damage taken do some sound or another for example
    virtual void OnHealed(int amount) {};
    virtual void PerformAttack() {};
    virtual void ShouldAttackTarget() {};
    void Die();

  protected:
    int maxHealth                               = 0;
    int currentHealth                           = 0;
    bool isInvulnerable                         = false;
    bool isDead                                 = false;
    int damage                                  = 0;
    float attackDuration                        = 0.0f;
    float speed                                 = 0.0f;
    float attackCooldown                        = 0.0f;
    float range                                 = 0.0f;
    AnimationComponent* animComponent           = nullptr;
    CapsuleColliderComponent* characterCollider = nullptr;
    std::string weaponName                      = "";
    GameObject* weapon                          = nullptr;
    CubeColliderComponent* weaponCollider       = nullptr;

    float lastAttackTime                        = -1.0f;
    float lastTimeHit                           = -1.0f;
    const float invulnerableDuration            = 0.7f;
    bool isAttacking                            = false;

    CharacterType type                          = CharacterType::None;

    // AI
    float rangeAIChase                          = 0.0f;
    float rangeAIAttack                         = 0.0f;
    float3 patrolPoint                          = float3::zero;
    bool reachedPatrolPoint                     = false;
    float3 startPos                             = float3::zero;
};
