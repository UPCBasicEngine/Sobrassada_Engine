#include "pch.h"

#include "Application.h"
#include "CameraComponent.h"
#include "Character.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "FireballTrap.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Mushroom.h"
#include "Projectile.h"
#include "ScriptComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "WindowModule.h"

#include <string>

Character::Character(
    GameObject* parent, int newMaxHealth, int newDamage, float newAttackDuration, float newAttackCooldown,
    float newRange, float newRangeAIAttack, float newRangeAIChase, CharacterType newType
)
    : Script(parent), maxHealth(newMaxHealth), attackDamage(newDamage), attackDuration(newAttackDuration),
      attackCooldown(newAttackCooldown), range(newRange), rangeAIAttack(newRangeAIAttack),
      rangeAIChase(newRangeAIChase), type(newType)
{
    currentHealth = maxHealth;

    fields.push_back({"Max Health", InspectorField::FieldType::Int, &maxHealth, 0, 10});
    fields.push_back({"Current Health", InspectorField::FieldType::Int, &currentHealth, 0, 10});
    fields.push_back({"Invulnerable", InspectorField::FieldType::Bool, &isInvulnerable, true, false});
    fields.push_back({"Dead", InspectorField::FieldType::Bool, &isDead, true, false});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &attackDamage, 0, 3});
    fields.push_back({"Attack Range", InspectorField::FieldType::Float, &range, 0.0f, 5.0f});
    fields.push_back({"Attack Duration", InspectorField::FieldType::Float, &attackDuration, 0.0f, 5.0f});
    fields.push_back({"Attack Cooldown", InspectorField::FieldType::Float, &attackCooldown, 0.0f, 5.0f});
    fields.push_back({"Attack Hitbox Delay", InspectorField::FieldType::Float, &attackHitboxDelay, 0.0f, 5.0f});
    fields.push_back({"Attack Hitbox Duration", InspectorField::FieldType::Float, &attackHitboxDuration, 0.0f, 5.0f});

    fields.push_back({"Heal Cooldown", InspectorField::FieldType::Float, &healCooldown, 0.0f, 5.0f});

    if (type != CharacterType::CuChulainn)
    {
        fields.push_back({"AI Chase Range", InspectorField::FieldType::Float, &rangeAIChase, 0.0f, 20.0f});
        fields.push_back({"AI Attack Range", InspectorField::FieldType::Float, &rangeAIAttack, 0.0f, 15.0f});
    }
}

bool Character::Init()
{
    animComponent = parent->GetComponent<AnimationComponent*>();
    if (!animComponent)
        GLOG(
            "Animation component not found for %s in character %s", parent->GetName().c_str(), parent->GetName().c_str()
        )
    else animComponent->OnPlay(false);

    characterCollider = parent->GetComponent<CapsuleColliderComponent*>();
    if (!characterCollider)
        GLOG(
            "Character capsule collider component not found for %s in character %s", parent->GetName().c_str(),
            parent->GetName().c_str()
        )

    weaponCollider = parent->GetComponentChild<CapsuleColliderComponent*>(AppEngine);

    if (!weaponCollider)
    {
        GLOG("[WARNING] No capsule weapon collider in child");
    }
    else
    {
        weapon = weaponCollider->GetParent();
        if (!weapon) GLOG("Weapon game object not found")
        else weaponCollider->SetEnabled(false);
    }

    startPos = parent->GetGlobalTransform().TranslatePart();

    return true;
}

void Character::Update(float deltaTime)
{
    if (isDead) return;

    if (!characterCollider || !weaponCollider || !weapon) return;

    // Get state name for debugging
    if (animComponent && stateName != animComponent->GetCurrentStateName())
    {
        stateName = animComponent->GetCurrentStateName();
        // GLOG("Current state: %s", stateName.GetString().c_str());
    }

    HandleState(deltaTime);
    UpdateTimers(deltaTime);
}

void Character::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // cube collider should be only if is enabled here already checked by OnCollision of cubeColliderComponent
    // GLOG("COLLISION %s with %s", parent->GetName().c_str(), otherObject->GetName().c_str())

    // ---- Damage Collisions ----

    // Melee check
    CapsuleColliderComponent* otherWeapon = otherObject->GetComponent<CapsuleColliderComponent*>();
    ScriptComponent* otherScript          = otherObject->GetComponentParent<ScriptComponent*>(AppEngine);

    if (otherScript && otherWeapon && otherWeapon->GetEnabled())
    {
        // Special attack check
        CuChulainn* playerScript = otherScript->GetScriptByType<CuChulainn>();
        if (playerScript && playerScript->GetState() == CharacterStates::ULTIMATE)
            TakeDamage(playerScript->GetUltimateDamage());

        // Standard attack check
        Character* enemyScript = otherScript->GetScriptByType<Character>();
        if (enemyScript)
        {
            if (!enemyScript->isAttacking) return;
            TakeDamage(enemyScript->attackDamage);
        }
    }

    if (otherWeapon && otherWeapon->GetEnabled() && otherObject->GetName() == "DarkPath")
    {
        TakeDamage(1);
    }

    otherScript = otherObject->GetComponent<ScriptComponent*>();
    if (otherScript)
    {
        // Projectile check
        Projectile* projectile = otherScript->GetScriptByType<Projectile>();
        if (projectile && otherWeapon && otherWeapon->GetEnabled())
        {
            TakeDamage(projectile->GetDamage());
            otherWeapon->SetEnabled(false);
            otherObject->SetEnabled(false);
        }

        // Trap check
        FireballTrap* fireballScript = otherScript->GetScriptByType<FireballTrap>();
        if (fireballScript)
        {
            SphereColliderComponent* damageCollider = otherObject->GetComponent<SphereColliderComponent*>();

            if (damageCollider && damageCollider->GetEnabled())
            {
                TakeDamage(fireballScript->GetDamage());
                damageCollider->SetEnabled(false);
            }
        }

        // Mushroom check
        Mushroom* mushroomScript = otherScript->GetScriptByType<Mushroom>();
        if (desiredHeal && mushroomScript)
        {
            if (mushroomScript->IsReady())
            {
                Heal(mushroomScript->GetHealingAmount());
                mushroomScript->Disable();
            }
        }
    }
}

void Character::Attack(float deltaTime)
{
    isAttacking = true;
    attackTimer = 0.0f;
}

void Character::UpdateTimers(float deltaTime)
{
    if (isAttacking) attackTimer += deltaTime;

    attackCdTimer -= deltaTime;
    if (attackCdTimer < 0.0f) attackCdTimer = 0.0f;

    if (isInvulnerable)
    {
        invulnerabilityTimer -= deltaTime;
        if (invulnerabilityTimer <= 0.0f) isInvulnerable = false;
    }

    healCdTimer -= deltaTime;
    if (healCdTimer <= 0.0f)
    {
        desiredHeal = false;
        healCdTimer = 0.0f;
    }
}

void Character::TakeDamage(int amount)
{
    if (isInvulnerable) return;

    currentHealth        -= amount;

    isInvulnerable        = true;
    invulnerabilityTimer  = invulnerableDuration;

    if (currentHealth <= 0) Die();
    else OnDamageTaken(amount);
}

void Character::Restart()
{
    if (characterCollider)
    {
        characterCollider = parent->GetComponent<CapsuleColliderComponent*>();
        characterCollider->SetEnabled(true);
    }

    if (weaponCollider)
    {
        weaponCollider = weapon->GetComponent<CapsuleColliderComponent*>();
        weaponCollider->SetEnabled(true);
    }

    parent->SetEnabled(true);
}

void Character::Heal(int amount)
{
    currentHealth += amount;

    if (currentHealth > maxHealth) currentHealth = maxHealth;

    OnHealed(amount);
}

PlayerDistances Character::CheckDistanceWithPlayer() const
{
    if (character != nullptr)
    {
        float distance = character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
        if (distance <= rangeAIAttack) return PlayerDistances::Close;
        else if (distance <= rangeAIChase) return PlayerDistances::Medium;
    }
    return PlayerDistances::Far;
}

bool Character::CheckDistanceWithPoint(const float3& point) const
{
    float3 parentPoint = parent->GetGlobalTransform().TranslatePart();
    parentPoint.y      = point.y;

    float distance     = parentPoint.Distance(point);
    if (distance <= 0.75f) return true;
    return false;
}

void Character::Die()
{
    // GLOG("%s dead", parent->GetName().c_str());
    isDead = true;
    OnDeath();

    if (characterCollider)
    {
        characterCollider->DeleteRigidBody();
        characterCollider->SetEnabled(false);
    }

    if (weaponCollider)
    {
        weaponCollider->DeleteRigidBody();
        weaponCollider->SetEnabled(false);
    }
}

void Character::RenderDebug(std::vector<std::pair<std::string, float2>> logs, float3 color)
{
    DebugDrawModule* debug        = AppEngine->GetDebugDrawModule();
    const CameraComponent* camera = AppEngine->GetSceneModule()->GetScene()->GetMainCamera();

    const float4 clipSpacePos     = camera->GetProjectionMatrix() * camera->GetViewMatrix() *
                                float4(parent->GetGlobalTransform().TranslatePart(), 1.0f);
    const float3 ndc = float3(clipSpacePos.x, clipSpacePos.y, clipSpacePos.z) / clipSpacePos.w;

#ifdef GAME
    float screenX = (ndc.x + 1.0f) * 0.5f * AppEngine->GetWindowModule()->GetWidth();
    float screenY = (1.0f - ndc.y) * 0.5f * AppEngine->GetWindowModule()->GetHeight();
#else
    const auto& windowSize = AppEngine->GetSceneModule()->GetScene()->GetWindowSize();
    const float screenX          = (ndc.x + 1.0f) * 0.5f * std::get<0>(windowSize);
    const float screenY          = (1.0f - ndc.y) * 0.5f * std::get<1>(windowSize);
#endif

    const float scale        = 0.6f;

    for (const auto& log : logs)
    {
        const float x = screenX + log.second.x;
        const float y = screenY + log.second.y;
        debug->Draw2DText(log.first.c_str(), float3(x, y, 0.0f), color, scale);
    }
}
