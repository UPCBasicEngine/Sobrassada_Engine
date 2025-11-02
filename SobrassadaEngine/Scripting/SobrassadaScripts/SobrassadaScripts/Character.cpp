#include "pch.h"

#include "Application.h"
#include "ArcherProjectile.h"
#include "AttackVfxSpritesheet.h"
#include "Banshee.h"
#include "Boss.h"
#include "CameraComponent.h"
#include "Character.h"
#include "ColorChange.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "FireballTrap.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "MagicBarrier.h"
#include "Math/Quat.h"
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
#include <math.h>

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

    if (type != CharacterType::CuChulainn && type != CharacterType::Mirage)
    {
        fields.push_back({"AI Chase Range", InspectorField::FieldType::Float, &rangeAIChase, 0.0f, 20.0f});
        fields.push_back({"AI Attack Range", InspectorField::FieldType::Float, &rangeAIAttack, 0.0f, 25.0f});
        fields.push_back({"AI Max Detection Range", InspectorField::FieldType::Float, &maxDetectionRange, 0.0f, 30.0f});
        fields.push_back({"Player search duration", InspectorField::FieldType::Float, &searchDuration, 0.0f, 10.0f});
        fields.push_back({"Mesh name", InspectorField::FieldType::InputText, &meshName});
        if (type == CharacterType::Boss || type == CharacterType::Soldier)
            fields.push_back({"Mesh 2 name", InspectorField::FieldType::InputText, &mesh2Name});
        if (type == CharacterType::Boss)
        {
            fields.push_back({"Mesh 3 name", InspectorField::FieldType::InputText, &mesh3Name});
            fields.push_back({"Mesh 4 name", InspectorField::FieldType::InputText, &mesh4Name});
        }
        fields.push_back({"On Hit VFX Duration", InspectorField::FieldType::Float, &onHitVfxDuration, 0.0f, 1.0f});
        fields.push_back({"On Hit Pivot Name", InspectorField::FieldType::InputText, &onHitPivotName});
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

    GameObject* meshObject = parent->GetChildGameObjectByName(meshName);
    if (meshObject)
    {
        mesh = meshObject->GetComponent<MeshComponent*>();
        if (mesh) mesh->SetEnabled(true);

        meshScripts = meshObject->GetComponent<ShaderScriptComponent*>();
        if (meshScripts) meshScripts->SetEnabled(false);
    }
    else
    {
        GLOG("[WARNING - %s] No mesh object found in children", parent->GetName().c_str())
    }

    if (type != CharacterType::CuChulainn && type != CharacterType::Mirage)
    {
        onHitPivot = parent->GetChildGameObjectByName(onHitPivotName);

        onHitVfx1  = parent->GetChildGameObjectByName(onHitVfx1Name);
        if (onHitVfx1) onHitVfx1->SetEnabled(false);

        onHitVfx2 = parent->GetChildGameObjectByName(onHitVfx2Name);
        if (onHitVfx2) onHitVfx2->SetEnabled(false);

        if (!mesh2Name.empty())
        {
            GameObject* mesh2Object = parent->GetChildGameObjectByName(mesh2Name);
            if (mesh2Object)
            {
                mesh2 = mesh2Object->GetComponent<MeshComponent*>();
                if (mesh2) mesh2->SetEnabled(true);
                // else GLOG("[WARNING - %s] No mesh component found", parent->GetName().c_str())

                color2Change = mesh2Object->GetComponent<ShaderScriptComponent*>();
                if (color2Change) color2Change->SetEnabled(false);
                // else GLOG("[WARNING - %s] No shader script component found", parent->GetName().c_str())
            }
            else
            {
                GLOG("[WARNING - %s] No mesh 2 object found in children", parent->GetName().c_str())
            }
        }

        if (!mesh3Name.empty())
        {
            GameObject* mesh3Object = parent->GetChildGameObjectByName(mesh3Name);
            if (mesh3Object)
            {
                mesh3 = mesh3Object->GetComponent<MeshComponent*>();
                if (mesh3) mesh3->SetEnabled(true);
                // else GLOG("[WARNING - %s] No mesh component found", parent->GetName().c_str())

                color3Change = mesh3Object->GetComponent<ShaderScriptComponent*>();
                if (color3Change) color3Change->SetEnabled(false);
                // else GLOG("[WARNING - %s] No shader script component found", parent->GetName().c_str())
            }
            else
            {
                GLOG("[WARNING - %s] No mesh 3 object found in children", parent->GetName().c_str())
            }
        }

        if (!mesh4Name.empty())
        {
            GameObject* mesh4Object = parent->GetChildGameObjectByName(mesh4Name);
            if (mesh4Object)
            {
                mesh4 = mesh4Object->GetComponent<MeshComponent*>();
                if (mesh4) mesh4->SetEnabled(true);
                // else GLOG("[WARNING - %s] No mesh component found", parent->GetName().c_str())

                color4Change = mesh4Object->GetComponent<ShaderScriptComponent*>();
                if (color4Change) color4Change->SetEnabled(false);
                // else GLOG("[WARNING - %s] No shader script component found", parent->GetName().c_str())
            }
            else
            {
                GLOG("[WARNING - %s] No mesh 4 object found in children", parent->GetName().c_str())
            }
        }

        GameObject* glowObject = parent->GetChildGameObjectByName(glowName);
        if (glowObject) glow = glowObject;
        if (!glowObject) GLOG("[WARNING - %s] No glow object found in children", parent->GetName())
    }

    startPos = parent->GetGlobalTransform().TranslatePart();

    return true;
}

void Character::Update(float deltaTime)
{
    if (isDead && type != CharacterType::Boss) return;

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

        Spouts* spoutsScript = otherScript->GetScriptByType<Spouts>();
        if (spoutsScript)
        {
            if (!spoutsScript->bossControlled)
            {
                TakeDamage(spoutsScript->GetDamage());
                spoutsScript->DisableCollider();
            }
        }
    }

    if (HashString(otherObject->GetName()) == HashString("BlastArea"))
    {
        ScriptComponent* otherScript = otherObject->GetComponentParent<ScriptComponent*>(AppEngine);
        if (otherScript)
        {
            Boss* bossScript = otherScript->GetScriptByType<Boss>();
            if (bossScript)
            {
                bossScript->DisableBlastArea();
                TakeDamage(1);
            }
        }
    }
}

void Character::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    // cube collider should be only if is enabled here already checked by OnCollision of cubeColliderComponent
    // GLOG("COLLISION %s with %s", parent->GetName().c_str(), otherObject->GetName().c_str())

    // ---- Damage Collisions ----

    // if (type == CharacterType::Boss) hitCollisionNormal = collisionNormal;
    hitGOFront                                 = otherObject->GetGlobalTransform().WorldZ().Normalized();
    hitCollisionNormal                         = collisionNormal.Normalized();

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
        else if (playerScript && (playerScript->GetState() == CharacterStates::HEAL || playerScript->GetState() == CharacterStates::TRANSFORM))
        {
            TakeDamage(0);
        }

        Character* enemyScript = otherScript->GetScriptByType<Character>();
        // Banshee slow area
        if (enemyScript && enemyScript->GetCharacterType() == CharacterType::Banshee)
        {
            CuChulainn* playerScript = parent->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>();
            Banshee* bansheeScript   = otherScript->GetScriptByType<Banshee>();

            if (playerScript && bansheeScript && bansheeScript->GetState() == BansheeStates::SlowArea)
            {
                TakeDamage(bansheeScript->GetSlowAreaDamage());
                playerScript->AddRiastrad(
                    -(bansheeScript->GetSlowAreaRiastradReduction() + playerScript->GetRiastradOnDamageTaken())
                );
                return;
            }
        }
        else if (enemyScript && enemyScript->GetCharacterType() == CharacterType::Boss)
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
        HashString(otherObject->GetName()) == HashString("DashTrailCollision"))
    {
        playerScript->StartCurse();
    }

    otherScript = otherObject->GetComponent<ScriptComponent*>();
    if (otherScript)
    {
        // Player projectile check
        Projectile* projectile = otherScript->GetScriptByType<Projectile>();
        if (projectile && otherWeapon && otherWeapon->GetEnabled())
        {
            TakeDamage(projectile->GetDamage());
            otherWeapon->SetEnabled(false);
            otherObject->SetEnabled(false);
        }

        // Archer projectile check
        ArcherProjectile* archerProjectile = otherScript->GetScriptByType<ArcherProjectile>();
        if (archerProjectile && otherWeapon && otherWeapon->GetEnabled())
        {
            if (type == CharacterType::CuChulainn)
            {
                CuChulainn* player = static_cast<CuChulainn*>(this);
                player->OnArrowHit();
            }
            TakeDamage(archerProjectile->GetDamage());
            archerProjectile->Reset();
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
            if (spoutsScript->bossControlled)
            {
                TakeDamage(spoutsScript->GetDamage());
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

    searchTimer -= deltaTime;
    if (searchTimer < 0.0f) searchTimer = 0.0f;

    if (isHit)
    {
        onHitVfxTimer -= deltaTime;

        if (onHitVfxTimer < 0.0f)
        {
            if (onHitVfx1 && onHitVfx1->IsEnabled()) onHitVfx1->SetEnabled(false);
            if (onHitVfx2 && onHitVfx2->IsEnabled()) onHitVfx2->SetEnabled(false);

            // Do this in the next frame after enabling the mesh to avoid popping
            if (mesh && mesh->GetEnabled() && meshScripts && meshScripts->GetEnabled())
            {
                meshScripts->SetEnabled(false);
                isHit = false;
            }

            if (mesh2 && mesh2->GetEnabled() && color2Change && color2Change->GetEnabled())
            {
                color2Change->SetEnabled(false);
                isHit = false;
            }
            if (mesh3 && mesh3->GetEnabled() && color3Change && color3Change->GetEnabled())
            {
                color3Change->SetEnabled(false);
                isHit = false;
            }
            if (mesh4 && mesh4->GetEnabled() && color4Change && color4Change->GetEnabled())
            {
                color4Change->SetEnabled(false);
                isHit = false;
            }

            if (mesh && !mesh->GetEnabled()) mesh->SetEnabled(true);
            if (mesh2 && !mesh2->GetEnabled()) mesh2->SetEnabled(true);
            if (mesh3 && !mesh3->GetEnabled()) mesh3->SetEnabled(true);
            if (mesh4 && !mesh4->GetEnabled()) mesh4->SetEnabled(true);
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

    if (meshScripts && mesh)
    {
        mesh->SetEnabled(false);
        meshScripts->SetEnabled(true);
    }

    isHit         = true;
    onHitVfxTimer = onHitVfxDuration;

    if (type != CharacterType::CuChulainn && type != CharacterType::Mirage)
    {
        if (type != CharacterType::Destructible) playerScript->OnEnemyHit();
        else playerScript->OnObjectDestroyed();

        if (onHitPivot)
        {
            float3 pivotPos  = onHitPivot->GetGlobalTransform().TranslatePart();
            float3 targetPos = character->GetParent()->GetGlobalTransform().TranslatePart();

            float3 dirWorld  = targetPos - pivotPos;
            dirWorld.y       = 0.0f;
            dirWorld.Normalize();

            float4x4 parentWorld = onHitPivot->GetParentGlobalTransform();
            float3 dirLocal      = parentWorld.Inverted().TransformDir(dirWorld);

            float localYaw       = atan2(dirLocal.x, dirLocal.z);
            Quat yawRot          = Quat::RotateY(localYaw);

            float4x4 local       = onHitPivot->GetLocalTransform();
            float4x4 newLocal    = float4x4::FromTRS(local.TranslatePart(), yawRot, local.GetScale());
            onHitPivot->SetLocalTransform(newLocal);
        }

        if (onHitVfx1)
        {
            onHitVfx1->SetEnabled(true);
            onHitVfx1->GetComponent<MeshComponent*>()->SetEnabled(false);
            onHitVfx1->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();
        }

        if (onHitVfx2)
        {
            onHitVfx2->SetEnabled(true);
            onHitVfx2->GetComponent<MeshComponent*>()->SetEnabled(false);
            onHitVfx2->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();
        }

        if (meshScripts && mesh)
        {
            mesh->SetEnabled(false);
            meshScripts->SetEnabled(true);
        }

        if (color2Change && mesh2)
        {
            mesh2->SetEnabled(false);
            color2Change->SetEnabled(true);
        }

        if (color3Change && mesh3)
        {
            mesh3->SetEnabled(false);
            color3Change->SetEnabled(true);
        }

        if (color4Change && mesh4)
        {
            mesh4->SetEnabled(false);
            color4Change->SetEnabled(true);
        }

        isHit         = true;
        onHitVfxTimer = onHitVfxDuration;
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

    if (type != CharacterType::CuChulainn && type != CharacterType::Destructible) playerScript->OnEnemyDefeated();

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
