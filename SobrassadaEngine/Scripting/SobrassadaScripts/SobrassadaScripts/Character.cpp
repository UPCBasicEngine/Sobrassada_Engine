#include "pch.h"

#include "Application.h"
#include "AttackVfxSpritesheet.h"
#include "Banshee_v2.h"
#include "Boss.h"
#include "CameraComponent.h"
#include "Character.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "FireballTrap.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "MagicBarrier.h"
#include "Mushroom.h"
#include "Projectile.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Spouts.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "WindowModule.h"

#include <string>

Character::Character(
    GameObject* parent, int newMaxHealth, int newDamage, float newAttackDuration, float newAttackCooldown,
    float newRange, float newRangeAIAttack, float newRangeAIChase, float newDetectionRange, CharacterType newType
)
    : Script(parent), maxHealth(newMaxHealth), attackDamage(newDamage), attackDuration(newAttackDuration),
      attackCooldown(newAttackCooldown), range(newRange), rangeAIAttack(newRangeAIAttack),
      rangeAIChase(newRangeAIChase), maxDetectionRange(newDetectionRange), type(newType)
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
        fields.push_back({"AI Max Detection Range", InspectorField::FieldType::Float, &maxDetectionRange, 0.0f, 15.0f});
        fields.push_back({"Player search duration", InspectorField::FieldType::Float, &searchDuration, 0.0f, 10.0f});
        fields.push_back({"On Hit VFX 1", InspectorField::FieldType::InputText, &onHitVfx1Name});
        fields.push_back({"On Hit VFX 2", InspectorField::FieldType::InputText, &onHitVfx2Name});
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

    if (type != CharacterType::CuChulainn)
    {
        onHitVfx1 = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(onHitVfx1Name);
        if (!onHitVfx1) GLOG("[WARNING] No on hit VFX found for enemy")
        else onHitVfx1->SetEnabled(false);

        onHitVfx2 = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(onHitVfx2Name);
        if (!onHitVfx2) GLOG("[WARNING] No on hit VFX found for enemy")
        else onHitVfx2->SetEnabled(false);
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
    }

    HandleState(deltaTime);
    UpdateTimers(deltaTime);
}

void Character::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    ScriptComponent* otherScript = otherObject->GetComponent<ScriptComponent*>();
    if (otherScript)
    {
        // Mushroom check
        Mushroom* mushroomScript = otherScript->GetScriptByType<Mushroom>();
        if (mushroomScript)
        {
            if (mushroomScript->IsReady() && playerScript->GetDesiredTakeMushroom() && playerScript->CanTakeMushroom())
            {
                if (playerScript->TakeMushroom()) mushroomScript->Disable();
            }
        }

        Projectile* arrowProj = otherScript->GetScriptByType<Projectile>();
        if (arrowProj)
        {
            arrowProj->Hit(otherObject);
        }
    }

    if (HashString(otherObject->GetName()) == HashString("BlastShield_2"))
    {
        TakeDamage(1);
    }
}

void Character::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // cube collider should be only if is enabled here already checked by OnCollision of cubeColliderComponent
    // GLOG("COLLISION %s with %s", parent->GetName().c_str(), otherObject->GetName().c_str())

    // ---- Damage Collisions ----

    // Melee check
    CapsuleColliderComponent* otherWeapon      = otherObject->GetComponent<CapsuleColliderComponent*>();
    SphereColliderComponent* otherWeaponShpere = otherObject->GetComponent<SphereColliderComponent*>();
    ScriptComponent* otherScript               = otherObject->GetComponentParent<ScriptComponent*>(AppEngine);

    if (otherScript && otherWeapon && otherWeapon->GetEnabled())
    {
        // Standard attack check
        Character* enemyScript = otherScript->GetScriptByType<Character>();
        if (enemyScript)
        {
            if (!enemyScript->isAttacking) return;

            TakeDamage(enemyScript->attackDamage);
        }
    }
    else if (otherScript && otherWeaponShpere && otherWeaponShpere->GetEnabled())
    {
        // Special attack check
        CuChulainn* playerScript = otherScript->GetScriptByType<CuChulainn>();
        if (playerScript && playerScript->GetState() == CharacterStates::ULTIMATE)
        {
            TakeDamage(playerScript->GetUltimateDamage());
        }
        // Charged attack check
        else if (playerScript && playerScript->GetState() == CharacterStates::CHARGED_ATTACK)
        {
            TakeDamage(playerScript->GetChargedAttackDamage());
        }

        // Heal & Riastrad knockback check
        else if (playerScript && (playerScript->GetState() == CharacterStates::HEAL ||
                                  playerScript->GetState() == CharacterStates::TRANSFORM))
        {
            TakeDamage(0);
        }

        Character* enemyScript = otherScript->GetScriptByType<Character>();
        // Banshee slow area
        if (enemyScript->GetCharacterType() == CharacterType::Banshee)
        {
            CuChulainn* playerScript  = parent->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>();
            Banshee_v2* bansheeScript = otherScript->GetScriptByType<Banshee_v2>();

            if (playerScript && bansheeScript && bansheeScript->GetState() == Banshee_v2_States::SlowArea)
            {
                playerScript->StartCurse();
                TakeDamage(bansheeScript->GetSlowAreaDamage());
                return;
            }
        }
        else if (enemyScript->GetCharacterType() == CharacterType::Boss)
        {
            Boss* bossScript = otherScript->GetScriptByType<Boss>();
            if (bossScript)
            {
                if (bossScript->GetCloseArea() &&
                    otherWeaponShpere == bossScript->GetCloseArea()->GetComponent<SphereColliderComponent*>())
                    TakeDamage(bossScript->GetCloseAreaDamage());
                else TakeDamage(enemyScript->attackDamage);
            }
        }
    }

    CubeColliderComponent* otherWeaponCube = otherObject->GetComponent<CubeColliderComponent*>();
    if (type == CharacterType::CuChulainn && otherWeaponCube && otherWeaponCube->GetEnabled() &&
        otherObject->GetName() == "DashTrailCollision")
    {
        playerScript->StartCurse();
    }

    otherScript = otherObject->GetComponent<ScriptComponent*>();
    if (otherScript)
    {
        // Projectile check
        Projectile* projectile = otherScript->GetScriptByType<Projectile>();
        if (projectile && otherWeapon && otherWeapon->GetEnabled())
        {

            if (type == CharacterType::CuChulainn)
            {
                CuChulainn* player = static_cast<CuChulainn*>(this);
                player->OnArrowHit();
            }

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

        Spouts* spoutsScript = otherScript->GetScriptByType<Spouts>();
        if (spoutsScript)
        {
            TakeDamage(spoutsScript->GetDamage());
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

    searchTimer -= deltaTime;
    if (searchTimer < 0.0f) searchTimer = 0.0f;


    if (type != CharacterType::CuChulainn)
    {
        GLOG("onHitVfxTimer: %f", onHitVfxTimer)
        onHitVfxTimer -= deltaTime;

        if (onHitVfxTimer < 0.0f)
        {
            // onHitVfxTimer = 0.0f;
            onHitVfx1->SetEnabled(false);
            onHitVfx2->SetEnabled(false);
        }
    }
}

void Character::TakeDamage(int amount)
{
    if (isInvulnerable) return;

    currentHealth        -= amount;

    isInvulnerable        = true;
    invulnerabilityTimer  = invulnerableDuration;

    OnDamageTaken(amount);

    if (type != CharacterType::CuChulainn)
    {
        playerScript->OnEnemyHit();

        onHitVfx1->SetEnabled(true);
        onHitVfx1->GetComponent<MeshComponent*>()->SetEnabled(false);
        onHitVfx1->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();

        onHitVfx2->SetEnabled(true);
        onHitVfx2->GetComponent<MeshComponent*>()->SetEnabled(false);
        onHitVfx2->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();

        GLOG("ENABLEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE")

        const float onHitVfxDuration = 0.1f;
        onHitVfxTimer                = onHitVfxDuration;
    }

    if (currentHealth <= 0) Die();
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

float Character::GetDistanceFromPlayer() const
{
    return character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
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

    if (type != CharacterType::CuChulainn) playerScript->OnEnemyDefeated();

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

    if (associatedBarrier != nullptr) associatedBarrier->EnemyDied();
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
    const float screenX    = (ndc.x + 1.0f) * 0.5f * std::get<0>(windowSize);
    const float screenY    = (1.0f - ndc.y) * 0.5f * std::get<1>(windowSize);
#endif

    const float scale = 0.6f;

    for (const auto& log : logs)
    {
        const float x = screenX + log.second.x;
        const float y = screenY + log.second.y;
        debug->Draw2DText(log.first.c_str(), float3(x, y, 0.0f), color, scale);
    }
}
