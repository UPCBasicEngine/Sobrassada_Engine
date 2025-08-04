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
    fields.push_back({"Is Static ", InspectorField::FieldType::Bool, &isStatic});
    
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
        agentAI->SetLookForward(false);
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
    if (!canEscape)
    {
        escapeUsedTimer += deltaTime;
        if (escapeUsedTimer >= ESCAPE_COOLDOWN) canEscape = true;
        
    }
     if (isKnockback)
    {
         float3 currentPos = parent->GetGlobalTransform().TranslatePart();
         GLOG("KNOCKBACK - Timer: %.2f, Pos: %.2f,%.2f,%.2f", knockbackTimer, currentPos.x, currentPos.y, currentPos.z);

         knockbackTimer      -= deltaTime;

        
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

    if (animComponent) animComponent->UseTrigger("damageSmall");
    
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
  
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("overdraw");
        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);

        // Reset machine gun variables
        currentShot        = 0;
        shotTimer          = 0.0f;
        hasStartedShooting = false;

       
    }
    else
    {
       

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

      
        if (!hasStartedShooting && attackTimer >= attackHitboxDelay)
        {
            hasStartedShooting = true;
            currentShot        = 0;
            shotTimer          = 0.0f;
            GLOG("OVERSHOOTING - MACHINE GUN SEQUENCE STARTED!");
        }

       
        if (hasStartedShooting && currentShot < numberOfShoots)
        {
            shotTimer += deltaTime;
            GLOG("MACHINE GUN - Shot %d/%d, Timer: %.2f/%.2f", currentShot + 1, numberOfShoots, shotTimer, shotDelay);

            if (shotTimer >= shotDelay)
            {
               
                float3 baseDirection = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
                baseDirection.Normalize();

                
                float spreadAngle     = 10.0f * (3.14159f / 180.0f); 
                float randomAngle     = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngle;

                float3 shootDirection = baseDirection;
                float cosA            = std::cos(randomAngle);
                float sinA            = std::sin(randomAngle);
                float x               = shootDirection.x * cosA - shootDirection.z * sinA;
                float z               = shootDirection.x * sinA + shootDirection.z * cosA;
                shootDirection.x      = x;
                shootDirection.z      = z;

               
                arrow->Shoot(parent->GetPosition(), shootDirection);
                currentShot++;
                shotTimer = 0.0f; 
            }
        }

       
        bool allShotsFired = (currentShot >= numberOfShoots);
        bool timeExpired   = (attackTimer >= attackDuration);

        if (allShotsFired || timeExpired)
        {
            GLOG("OVERSHOOTING FINISHED - Machine gun sequence complete!");
            hasShot            = false;
            isAttacking        = false;
            hasStartedShooting = false;
            currentShot        = 0;
            shotTimer          = 0.0f;
            attackCdTimer      = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->SetLookForward(true);
            isAiming = false;
            aimTimer = 0.0f;

            ChangeState();
            return;
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

        if (deathTimer >= DEATH_DURATION)
        {
            parent->SetEnabled(false); 
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
    if (isStatic)
    {
        currentState = ArcherStates::SEARCH;
        return;
    }

    if (animComponent) animComponent->UseTrigger("idle");

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
  

    if (isStatic)
    {
      
        if (animComponent) animComponent->UseTrigger("idle");

        float distance = GetDistanceFromPlayer();
        if (distance <= maxDetectionRange)
        {
            if (character && agentAI)
            {
                agentAI->SetSpeed(0.0f, 0.0f);                                
                agentAI->LookAtMovement(character->GetLastPosition(), 0.016f); 
               
            }

            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f)
            {
                
                currentState = ArcherStates::AIM;
            }
        }
        else
        {
            if (agentAI) agentAI->SetSpeed(0.0f, 0.0f);
        }
    }
    else
    {
        if (!isSearching)
        {
            animComponent->UseTrigger("idle");
            isSearching = true;
            searchTimer = searchDuration;
            agentAI->SetSpeed(0.0f, 0.0f);
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
   
}

void Archer::Aim(float deltaTime)
{

  float distance = GetDistanceFromPlayer();
    if (!weaponCollider) return;

    if (!isAiming)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("aim");

        isAiming = true;
        aimTimer = 0.0f;
        //agentAI->SetSpeed(0.0f, 0.0f);
    }
    else
    {
        aimTimer += deltaTime;
      

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
             currentState = ArcherStates::OVERSHOOTING;
            }
            else
            {
               
                currentState = ArcherStates::BASIC_ATTACK;
            }
        }
    }

    
        
  
}

void Archer::Attack(float deltaTime)
{
    float distance = GetDistanceFromPlayer();
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("attack");
        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);
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
    if (isDead) return;
    
    if (playerScript->IsDead())
    {
        GLOG("PLAYER IS DEAD - GOING TO PATROL");
        currentState = ArcherStates::PATROL;
        escapeCount  = 0;
        canEscape    = true;
        return;
    }

    const float distance = GetDistanceFromPlayer();
   

  if (isStatic)
    {
      if (distance <= maxDetectionRange)
      {
          if (distance <= rangeAIAttack && attackCdTimer <= 0.0f) currentState = ArcherStates::AIM;
          else if (distance <= rangeAIAttack && attackCdTimer > 0.0f) currentState = ArcherStates::SEARCH;
          else currentState = ArcherStates::SEARCH;
          
      }
      else
      {
          GLOG("STATIC ARCHER - NO PLAYER IN RANGE, IDLE");
          currentState = ArcherStates::SEARCH;
      }
    }
    else
    {
        if (distance <= rangeEscape && canEscape && escapeCount < MAX_ESCAPES)
        {
            currentState = ArcherStates::ESCAPE;
            escapeCount++;
            GLOG("ESCAPE COUNT %d: ", escapeCount);
            canEscape       = false;
            escapeUsedTimer = 0.0f;
        }
        else if (distance <= rangeAIAttack) currentState = ArcherStates::AIM;
        else if (distance >= rangeAIChase) currentState = ArcherStates::CHASE;
        else if (distance > maxDetectionRange) currentState = ArcherStates::SEARCH;
        else currentState = ArcherStates::PATROL;
        
    }
   
}

void Archer::Escape(float deltaTime)
{
    GLOG("ESCAPE STATE");
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