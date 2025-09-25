#pragma once

#include "HashString.h"
#include "Script.h"

#include <unordered_set>
#include <vector>

class MagicBarrier;
class GameObject;
class CharacterControllerComponent;
class AnimationComponent;
class CapsuleColliderComponent;
class ShaderScriptComponent;
class MeshComponent;

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
    Banshee,
    Destructible,
    Changeling,
    Boss,
    Mirage
};

class Character : public Script
{
  public:
    Character(
        GameObject* parent, int maxHealth, int damage, float attackDuration, float cooldown, float range,
        float rangeAIAttack, float rangeAIChase, float maxDetectionRange, CharacterType type
    );
    virtual ~Character() noexcept override { parent = nullptr; };

    virtual bool Init() override;
    virtual void Update(float deltaTime) override;
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

    virtual void TakeDamage(int amount);
    void Restart();
    bool IsDead() const { return isDead; };

    virtual void PlayHighlightSequence() {}

    CharacterType GetCharacterType() const { return type; }
    int GetMaxHealth() const { return maxHealth; }
    int GetCurrentHealth() const { return currentHealth; }

    void SetAssociatedBarrier(MagicBarrier* newAssociatedBarrier) { associatedBarrier = newAssociatedBarrier; }

  protected:
    virtual void Attack(float deltaTime);
    virtual void UpdateTimers(float deltaTime);
    void Heal(int amount);
    float GetDistanceFromPlayer() const;
    PlayerDistances CheckDistanceWithPlayer() const;
    bool CheckDistanceWithPoint(const float3& point) const;
    void RenderDebug(std::vector<std::pair<std::string, float2>> logs, float3 color);
    virtual void Die();

  private:
    virtual void HandleState(float deltaTime) {};
    virtual void OnDeath() {};
    virtual void OnDamageTaken(int amount) {}; // depending of amout damage taken do some sound or another for example
    virtual void OnHealed(int amount) {};
    virtual void PerformAttack() {};
    virtual void ShouldAttackTarget() {};

  protected:
    AnimationComponent* animComponent           = nullptr;
    CapsuleColliderComponent* characterCollider = nullptr;
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
    bool isInCountRange                         = false;
    float attackHitboxDelay                     = 0.0f;
    float attackHitboxDuration                  = 0.0f;

    float invulnerabilityTimer                  = 0.0f;
    const float invulnerableDuration            = 0.2f;

    bool desiredHeal                            = false;
    float healCooldown                          = 1.0f;
    float healCdTimer                           = 0.0f;

    CharacterType type                          = CharacterType::None;
    HashString stateName                        = HashString("");

    // AI
    float rangeAIChase                          = 0.0f;
    float rangeAIAttack                         = 0.0f;
    float maxDetectionRange                     = 0.0f;
    bool reachedPatrolPoint                     = false;
    float3 startPos                             = float3::zero;

    float searchTimer                           = 0.0f;
    float searchDuration                        = 5.0f;
    bool isSearching                            = false;

    // Hit VFX
    bool isHit                                  = false;
    float onHitVfxDuration                      = 0.1f;
    float onHitVfxTimer                         = 0.0;
    std::string onHitPivotName                  = "OnHitPivot";
    std::string onHitVfx1Name                   = "OnHitVfx1";
    std::string onHitVfx2Name                   = "OnHitVfx2";
    GameObject* onHitPivot                      = nullptr;
    GameObject* onHitVfx1                       = nullptr;
    GameObject* onHitVfx2                       = nullptr;

    std::string meshName                        = "";
    MeshComponent* mesh                         = nullptr;
    ShaderScriptComponent* colorChange          = nullptr;

    // Level
    MagicBarrier* associatedBarrier             = nullptr;
};