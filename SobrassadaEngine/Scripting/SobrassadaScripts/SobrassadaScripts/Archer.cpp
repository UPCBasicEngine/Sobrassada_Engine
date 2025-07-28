#include "pch.h"

#include "Application.h"
#include "Archer.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "Projectile.h"
#include "ResourceStateMachine.h"
#include "ScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include <cmath>

Archer::Archer(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Archer)
{
    fields.push_back({"AI Patrol Point", InspectorField::FieldType::Vec3, &patrolPoint, -1000.0f, 1000.0f});
    fields.push_back({"Arrow Projectile Name", InspectorField::FieldType::InputText, &arrowName});
    fields.push_back({"Escape Range", InspectorField::FieldType::Float, &rangeEscape, 0.0f, 10.0f});
    fields.push_back({"Aim Duration", InspectorField::FieldType::Float, &aimDuration, 0.0f, 5.0f});
    fields.push_back({"Has Multiple Shoots ", InspectorField::FieldType::Bool, &hasMultipleShoots});
    fields.push_back({"Number of Shoots", InspectorField::FieldType::Int, &numberOfShoots, 1, 5});
    fields.push_back({"Knockback Time", InspectorField::FieldType::Float, &knockbackTime, 0.0f, 1.0f});
    fields.push_back({"Knockback Force", InspectorField::FieldType::Float, &knockbackForce, 0.0f, 20.0f});
    
}

bool Archer::Init()
{
    // GLOG("Initiating Soldier");

    currentState = ArcherStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Archer")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    // Get the arrow. This assumes the archer has only one sibling, which is the arrow. If it has more probably will
    // keep working as long as the arrow is the second gameObject
    const GameObject* root           = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    const std::vector<UID>& siblings = root->GetChildren();
    GameObject* arrowObj             = nullptr;
    for (UID objectUID : siblings)
    {
        if (objectUID != parent->GetUID())
            arrowObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(objectUID);
    }

    if (arrowObj && arrowObj->GetComponent<ScriptComponent*>())
    {
        arrow = arrowObj->GetComponent<ScriptComponent*>()->GetScriptByType<Projectile>();
        if (!arrow) GLOG("[WARNING] No arrow found in archer");
    }

    return true;
}

void Archer::Update(float deltaTime)
{
    if (agentAI == nullptr) return;
    if (!parent->IsEnabled() && !isDead)
    {
        GLOG("CRITICAL BUG - Archer disabled but NOT dead! Health: %d", currentHealth);
        return;
    }
     if (isKnockback)
    {
         float3 currentPos = parent->GetGlobalTransform().TranslatePart();
         GLOG("KNOCKBACK - Timer: %.2f, Pos: %.2f,%.2f,%.2f", knockbackTimer, currentPos.x, currentPos.y, currentPos.z);

         knockbackTimer      -= deltaTime;

         // ← REEMPLAZAR agentAI->MoveTo() con movimiento directo:
         float3 movement      = knockbackDirection * knockbackForce * deltaTime;
         float4x4 transform  = parent->GetGlobalTransform();
         transform.SetTranslatePart(currentPos + movement);
         parent->SetLocalTransform(transform);

         if (knockbackTimer <= 0.0f)
         {
             GLOG("KNOCKBACK FINISHED - Final pos: %.2f,%.2f,%.2f", currentPos.x, currentPos.y, currentPos.z);
             isKnockback = false;
             agentAI->ResetSpeed();
             agentAI->ResetAngularSpeed();
             ChangeState();
         }
        return;
    }
    Character::Update(deltaTime);
   
    if (isDead)
    {
        GLOG("ARCHER IS DEAD - Only processing death");
        if (currentState == ArcherStates::DEATH && animComponent && animComponent->IsFinished())
        {
            GLOG("DEATH ANIMATION FINISHED - DISABLING OBJECT");
            parent->SetEnabled(false);
        }
      
    }
    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life      = "Health: " + std::to_string(currentHealth);
        const std::string animState = "Anim state: " + stateName.GetString();

        std::vector<std::pair<std::string, float2>> logs {
            {life,      float2(-50.0f, -140.0f)},
            {animState, float2(-80.0f, -160.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

void Archer::OnPlayerExitLocation()
{
    currentState = ArcherStates::PATROL;
    agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
}

void Archer::OnPlayerEnterLocation()
{
    currentState = ArcherStates::SEARCH;
}

void Archer::OnDeath()
{
        // TODO: include death sound for the character
    GLOG("ARCHER DYING - Going to DEATH state");
    isAttacking  = false;
    currentState = ArcherStates::DEATH;

    if (animComponent)
    {
        GLOG("TRIGGERING die ANIMATION");
        animComponent->UseTrigger("die");
    }
       
    // TODO: animation and particles
    
}

void Archer::OnDamageTaken(int amount)
{
    GLOG("TAKING DAMAGE - Setting knockback = true");
    isAttacking = false;
    attackTimer = 0.0f;
    isAiming    = false;
    aimTimer    = 0.0f;

    if (weaponCollider && weaponCollider->GetEnabled())
    {
        weaponCollider->SetEnabled(false);
    }

    isKnockback    = true;
    knockbackTimer = knockbackTime;
    ApplyKnockback();

    if (animComponent)
    {
        GLOG("TRIGGERING damageSmall ANIMATION");
        animComponent->UseTrigger("damageSmall");
    }
    // TODO: play soldier take damage sound
    // TODO: particles? and animation
}

void Archer::PerformAttack()
{
    
    // TODO: activate and disable the box collider located on one on the gameobjects weapon
    // TODO: trails, particles and animation
}

void Archer::OverShooting(float deltaTime)
{
    GLOG("ENTERING OVERSHOOTING");
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("overdraw");
        Character::Attack(deltaTime);
        //agentAI->SetSpeed(0.0f, 0.0f);
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            hasShot          = true;
            float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
            direction.Normalize();
            for (int i = 0; i < numberOfShoots; ++i)
            {
                arrow->Shoot(parent->GetPosition(), direction);
            }
            
        }

        if (attackTimer >= attackDuration)
        {
            GLOG("OVERSHOOTING FINISHED");
            hasShot       = false;
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->SetLookForward(true);

            isAiming = false;
            aimTimer = 0.0f;

            ChangeState();
        }
    }
}

void Archer::HandleState(float deltaTime)
{

      switch (currentState)
    {
    case ArcherStates::SEARCH:
        SearchForPlayer();
        break;
    case ArcherStates::PATROL:
        PatrolAI();
        break;
    case ArcherStates::CHASE:
        ChaseAI();
        break;
    case ArcherStates::AIM:
        Aim(deltaTime);
        break;
    case ArcherStates::BASIC_ATTACK:
        Attack(deltaTime); 
        break;
    case ArcherStates::OVERSHOOTING:
        OverShooting(deltaTime);
        break;
    case ArcherStates::ESCAPE:
        Escape(deltaTime);
        break;
    case ArcherStates::DEATH:
        
        deathTimer                 += deltaTime;
        GLOG("ARCHER IS DEAD - Death timer: %.2f", deathTimer);

        if (deathTimer >= DEATH_DURATION)
        {
            GLOG("ARCHER DESTROYED - Removing from scene");
            parent->SetEnabled(false); // o el método que uses para destruir
            return;
        }
        break;
        
    default:
        GLOG("No state provided to Archer");
        currentState = ArcherStates::PATROL;
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        //animComponent->UseTrigger("idle");
    }
}

void Archer::PatrolAI()
{
    if (animComponent) animComponent->UseTrigger("run");

    const HashString& playerLocation = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    bool playerInLocation            = parent->HasTag(playerLocation);

    if (!playerScript->IsDead())
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Medium && playerInLocation)
            currentState = ArcherStates::CHASE;
        else if (CheckDistanceWithPlayer() == PlayerDistances::Close && playerInLocation)
            currentState = ArcherStates::AIM;
    }

    bool valid = false;
    if (reachedPatrolPoint)
    {
        if (CheckDistanceWithPoint(startPos)) reachedPatrolPoint = false;
        else valid = agentAI->SetPathNavigation(startPos);
    }
    else
    {
        if (CheckDistanceWithPoint(patrolPoint)) reachedPatrolPoint = true;
        else valid = agentAI->SetPathNavigation(patrolPoint);
    }
 }

    
    
void Archer::ApplyKnockback()
 {
    float3 myPos         = parent->GetGlobalTransform().TranslatePart();
    knockbackDirection   = character->GetFrontDirection();
    knockbackDirection.y = 0.0f;
    if (knockbackDirection.LengthSq() < 0.001f) knockbackDirection = float3::unitZ;
    knockbackDirection.Normalize();
   
     GLOG("ApplyKnockback - MyPos: %.2f,%.2f,%.2f", myPos.x, myPos.y, myPos.z);
    GLOG(
        "ApplyKnockback - Direction: %.2f,%.2f,%.2f", knockbackDirection.x, knockbackDirection.y, knockbackDirection.z
    );
    GLOG("ApplyKnockback - Force: %.2f, Time: %.2f", knockbackForce, knockbackTime);
 }

void Archer::ChaseAI()
{
    if (animComponent) animComponent->UseTrigger("run");

    if (character != nullptr)
    {
        agentAI->SetPathNavigation(character->GetLastPosition());
        ChangeState();
    }
    else currentState = ArcherStates::PATROL;
}

void Archer::SearchForPlayer()
{
    GLOG("ENTERING SEARCH");
    if (!isSearching)
    {
        animComponent->UseTrigger("idle");
        isSearching = true;
        searchTimer = searchDuration;
        //agentAI->SetSpeed(0.0f, 0.0f);
    }

    if (GetDistanceFromPlayer() < maxDetectionRange - 0.5f)
    {
        isSearching = false;
        agentAI->ResetSpeed();
        currentState = ArcherStates::CHASE;
    }
    else if (searchTimer <= 0.0f)
    {
        isSearching  = false;
        currentState = ArcherStates::PATROL;
        agentAI->ResetSpeed();
    }
}

void Archer::Aim(float deltaTime)
{

    GLOG("ENTERING AIM STATE");
    if (!weaponCollider) return;

    if (!isAiming)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("aim");

        isAiming = true;
        aimTimer = 0.0f;
        //agentAI->SetSpeed(0.0f, 0.0f);
        GLOG("STARTING AIM - Duration: %.2f", aimDuration);
    }
    else
    {
        aimTimer += deltaTime;
        GLOG("AIM Timer: %.2f / %.2f", aimTimer, aimDuration);

        if (character)
        {
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        }

        if (aimTimer >= aimDuration)
        {
            isAiming = false;
            aimTimer = 0.0f;

           
            if (hasMultipleShoots)
            {
                GLOG("AIM FINISHED - GOING TO OVERSHOOTING");
                currentState = ArcherStates::OVERSHOOTING;
            }
            else
            {
                GLOG("AIM FINISHED - GOING TO BASIC_ATTACK");
                currentState = ArcherStates::BASIC_ATTACK;
            }
        }
    }
        
  
}

void Archer::Attack(float deltaTime)
{
    GLOG("ENTER TO ATTACK ");
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("attack");
        Character::Attack(deltaTime);
        //agentAI->SetSpeed(0.0f, 0.0f);
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            hasShot          = true;
            float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
            direction.Normalize();
            arrow->Shoot(parent->GetPosition(), direction);
        }

        if (attackTimer >= attackDuration)
        {
            hasShot       = false;
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->SetLookForward(true);

          
            isAiming = false;
            aimTimer = 0.0f;

            ChangeState(); 
        }
    }
}

void Archer::ChangeState()
{
    if (isDead) 
    {
        GLOG("ARCHER IS DEAD - NOT CHANGING STATE, keeping DEATH");
        return;
    }
    if (playerScript->IsDead())
    {
        GLOG("PLAYER IS DEAD - GOING TO PATROL");
        currentState = ArcherStates::PATROL;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    GLOG("ChangeState - Distance: %.2f, rangeEscape: %.2f, rangeAIAttack: %.2f", distance, rangeEscape, rangeAIAttack);

  
    if (distance <= rangeAIAttack)
    {
        GLOG("GOING TO AIM");
        currentState = ArcherStates::AIM;
    }
    else if (distance < rangeEscape)
    {
        GLOG("GOING TO ESCAPE");
        currentState = ArcherStates::ESCAPE;
    }
    else if (distance <= rangeAIChase)
    {
        GLOG("GOING TO CHASE");
        currentState = ArcherStates::CHASE;
    }
    else if (distance > maxDetectionRange)
    {
        GLOG("GOING TO SEARCH");
        currentState = ArcherStates::SEARCH;
    }
    else
    {
        GLOG("GOING TO PATROL");
        currentState = ArcherStates::PATROL;
    }
}

void Archer::Escape(float deltaTime)
{
    GLOG("ENTERING ESCAPE");

    if (!agentAI || !character) return;

    float3 archerPos        = parent->GetGlobalTransform().TranslatePart();
    const float3 searchArea = {1.0f, 2.0f, 1.0f};
    bool posOverPoly        = false;
    float3 closestPoint     = float3::zero;

    if (hasEscapeTarget)
    {
        if ((archerPos - currentEscapeTarget).LengthSq() < 0.5f * 0.5f)
        {
            hasEscapeTarget = false;
        }
        else
        {
            agentAI->GetClosestPointInNavmesh(currentEscapeTarget, searchArea, posOverPoly, closestPoint);
            if (posOverPoly)
            {
                agentAI->SetPathNavigation(currentEscapeTarget);
                agentAI->LookAtMovement(currentEscapeTarget, deltaTime);
                if (animComponent) animComponent->UseTrigger("run");
                if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) >= rangeEscape)
                {
                    hasEscapeTarget = false;
                    ChangeState(); 
                }
                return;
            }
            else
            {
                hasEscapeTarget = false;
            }
        }
    }

    const float3 playerPos = character->GetLastPosition();
    float3 escapeDir       = archerPos - playerPos;
    escapeDir.y            = 0.0f;
    if (escapeDir.LengthSq() < 0.0001f) escapeDir = float3::unitZ;
    escapeDir.Normalize();

    float escapeDistance =
        rangeAIAttack - character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
    const float angleStep = 15.0f * (3.14159265f / 180.0f);
    float angleAccum      = 0.0f;
    bool found            = false;

    for (int i = 0; i < 24; ++i)
    {
        float3 dir = escapeDir;
        float cosA = std::cos(angleAccum);
        float sinA = std::sin(angleAccum);
        float x    = dir.x * cosA - dir.z * sinA;
        float z    = dir.x * sinA + dir.z * cosA;
        dir.x      = x;
        dir.z      = z;
        dir.Normalize();

        float3 candidateTarget = archerPos + dir * escapeDistance;
        agentAI->GetClosestPointInNavmesh(candidateTarget, searchArea, posOverPoly, closestPoint);

        if (posOverPoly)
        {
            currentEscapeTarget = closestPoint;
            hasEscapeTarget     = true;
            found               = true;
            break;
        }
        angleAccum += angleStep;
    }

    if (!found)
    {
        currentEscapeTarget = archerPos;
        hasEscapeTarget     = false;
    }

    agentAI->SetPathNavigation(currentEscapeTarget);
    agentAI->LookAtMovement(currentEscapeTarget, deltaTime);
    agentAI->SetSpeed(5.0f, 5.0f);

    if (animComponent) animComponent->UseTrigger("run");

    if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) >= rangeEscape)
    {
        hasEscapeTarget = false;
        agentAI->ResetSpeed();
        ChangeState(); 
    }
}