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
    fields.push_back({"Shot Delay Duration", InspectorField::FieldType::Float, &shotDelay, 0.0f, 1.0f});
    fields.push_back({"Has Multiple Shoots ", InspectorField::FieldType::Bool, &hasMultipleShoots});
    fields.push_back({"Number of Shoots", InspectorField::FieldType::Int, &numberOfShoots, 1, 5});
    fields.push_back({"Knockback Time", InspectorField::FieldType::Float, &knockbackTime, 0.0f, 1.0f});
    fields.push_back({"Knockback Force", InspectorField::FieldType::Float, &knockbackForce, 0.0f, 20.0f});
    fields.push_back({"Is Static ", InspectorField::FieldType::Bool, &isStatic});
}

bool Archer::Init()
{
    currentState = ArcherStates::PATROL;
    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr)
    {
        GLOG("AIAgent component not found for Archer");
        return false;
    }
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(false);
        speed = agentAI->GetSpeed();
    }

    GLOG("=== ARCHER INIT START ===");
    GLOG("Archer: %s", parent->GetName().c_str());
    GLOG("Static: %s, Multiple Shoots: %s", isStatic ? "YES" : "NO", hasMultipleShoots ? "YES" : "NO");

    const GameObject* root           = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    const std::vector<UID>& siblings = root->GetChildren();

    GLOG("Parent: %s, Children: %d", root->GetName().c_str(), siblings.size());

   
    std::vector<GameObject*> allArrows;

    for (UID objectUID : siblings)
    {
        GameObject* obj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(objectUID);
        if (obj != parent)
        {
            std::string objName = obj->GetName();
            if (objName.find("Arrow_") != std::string::npos)
            {
                GLOG("Found arrow: %s", objName.c_str());

                ScriptComponent* scriptComp = obj->GetComponent<ScriptComponent*>();
                if (scriptComp)
                {
                    Projectile* projectile = scriptComp->GetScriptByType<Projectile>();
                    if (projectile)
                    {
                        allArrows.push_back(obj);
                        arrowPool.push_back(projectile);

                        GLOG("Arrow added to pool: %s (Total: %d)", objName.c_str(), arrowPool.size());
                    }
                }
            }
        }
    }

    GLOG("Total arrows found: %d", allArrows.size());

    
    if (hasMultipleShoots)
    {
        GLOG("=== MULTIPLE SHOOTS ARCHER ===");
      

      
        for (GameObject* arrowObj : allArrows)
        {
            arrowObj->SetEnabledRecursive(false);
            GLOG("Disabled arrow: %s", arrowObj->GetName().c_str());
        }
    }
    else
    {
        GLOG("=== SINGLE SHOOT ARCHER ===");

     
        if (!allArrows.empty())
        {
          
            GameObject* singleArrowObj = allArrows[0];
            arrow                      = arrowPool[0];

            GLOG("Single arrow assigned: %s", singleArrowObj->GetName().c_str());

            for (GameObject* arrowObj : allArrows)
            {
                arrowObj->SetEnabledRecursive(false);
                GLOG("Disabled arrow: %s", arrowObj->GetName().c_str());
            }
        }
        else
        {
            GLOG("[WARNING] No arrows found for single shoot archer");
        }
    }

  
    GLOG("=== FINAL ARROW STATES ===");
    for (int i = 0; i < arrowPool.size(); i++)
    {
        GameObject* arrowObj = arrowPool[i]->GetParent();
        GLOG("Arrow[%d]: %s, Enabled: %s", i, arrowObj->GetName().c_str(), arrowObj->IsEnabled() ? "YES" : "NO");
    }

    GLOG("=== ARCHER INIT COMPLETE ===");
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
      
        knockbackTimer     -= deltaTime;

        float3 movement     = knockbackDirection * knockbackForce * deltaTime;
        float4x4 transform  = parent->GetGlobalTransform();
        transform.SetTranslatePart(currentPos + movement);
        parent->SetLocalTransform(transform);

        if (knockbackTimer <= 0.0f)
        {
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

      
        currentShot        = 0;
        shotTimer          = 0.0f;
        hasStartedShooting = false;
        currentArrowIndex  = 0; 

        GLOG("OVERSHOOTING STARTED - Pool: %d, Target: %d", arrowPool.size(), numberOfShoots);
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
                if (arrowPool.empty())
                {
                    GLOG("[ERROR] Arrow pool is empty!");
                    isAttacking        = false;
                    hasStartedShooting = false;
                    ChangeState();
                    return;
                }

               
                float3 baseDirection = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
                baseDirection.Normalize();

                float spreadAngle           = 10.0f * (3.14159f / 180.0f);
                float randomAngle           = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngle;

                float3 shootDirection       = baseDirection;
                float cosA                  = std::cos(randomAngle);
                float sinA                  = std::sin(randomAngle);
                float x                     = shootDirection.x * cosA - shootDirection.z * sinA;
                float z                     = shootDirection.x * sinA + shootDirection.z * cosA;
                shootDirection.x            = x;
                shootDirection.z            = z;

                float3 arrowPos             = float3(parent->GetPosition().x, 1.3f, parent->GetPosition().z);

               
                Projectile* currentArrow    = arrowPool[currentArrowIndex];
                GameObject* arrowGameObject = currentArrow->GetParent();

                GLOG(
                    "FIRING ARROW %d - Index: %d, Arrow: %s", currentShot + 1, currentArrowIndex,
                    arrowGameObject ? arrowGameObject->GetName().c_str() : "NULL"
                );

                if (arrowGameObject)
                {
                  
                   
                    arrowGameObject->SetEnabled(true);
                    arrowGameObject->SetEnabledRecursive(true);

                    bool isEnabled = arrowGameObject->IsEnabled();
                    GLOG("Arrow activation result: %s", isEnabled ? "SUCCESS" : "FAILED");

                    currentArrow->Shoot(arrowPos, shootDirection);
                    GLOG("Arrow shot executed!");
                }
                else
                {
                    GLOG("[ERROR] Arrow GameObject is NULL!");
                }

             
                currentShot++;
                currentArrowIndex = (currentArrowIndex + 1) % arrowPool.size();
                shotTimer         = 0.0f;

                GLOG("Shot %d/%d COMPLETED - Next arrow index: %d", currentShot, numberOfShoots, currentArrowIndex);
            }
        }

        bool allShotsFired = (currentShot >= numberOfShoots);
        bool timeExpired   = (attackTimer >= attackDuration);

        if (allShotsFired || timeExpired)
        {
            GLOG(
                "OVERSHOOTING FINISHED - Shots fired: %d/%d, Time expired: %s", currentShot, numberOfShoots,
                timeExpired ? "YES" : "NO"
            );

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

        deathTimer += deltaTime;

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
        // animComponent->UseTrigger("idle");
    }
}

void Archer::PatrolAI()
{
    if (isStatic)
    {
        currentState = ArcherStates::AIM;
        return;
    }

    if (animComponent) animComponent->UseTrigger("idle");

    const HashString& playerLocation = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    bool playerInLocation            = parent->HasTag(playerLocation);

    if (!playerScript->IsDead() && playerScript->GetState() != CharacterStates::RESPAWN)
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
        // agentAI->SetSpeed(0.0f, 0.0f);
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

        GLOG("SINGLE ATTACK STARTED");
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            hasShot = true;

            if (!arrow)
            {
                GLOG("[ERROR] No arrow for single attack!");
                return;
            }

            float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
            direction.Normalize();
            float3 arrowPos      = float3(parent->GetPosition().x, 1.3f, parent->GetPosition().z);

            
            GameObject* arrowObj = arrow->GetParent();
            if (arrowObj)
            {
                GLOG("Single arrow before activation - Enabled: %s", arrowObj->IsEnabled() ? "YES" : "NO");

                arrowObj->SetEnabled(true);
                arrowObj->SetEnabledRecursive(true);

                bool isEnabled = arrowObj->IsEnabled();
                GLOG("Single arrow activation result: %s", isEnabled ? "SUCCESS" : "FAILED");
            }

            GLOG("SINGLE ARROW FIRING");
            arrow->Shoot(arrowPos, direction);
            GLOG("SINGLE ARROW SHOT COMPLETED");
        }

        if (attackTimer >= attackDuration)
        {
            GLOG("SINGLE ATTACK FINISHED");
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

    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
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
            else currentState = ArcherStates::AIM;
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