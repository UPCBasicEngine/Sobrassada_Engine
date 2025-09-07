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
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Wwise_IDs.h"
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
        /*  GLOG("AIAgent component not found for Archer");
          return false;*/
    }
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(false);
        speed = agentAI->GetSpeed();
    }

    /* GLOG("=== ARCHER INIT START ===");
     GLOG("Archer: %s", parent->GetName().c_str());
     GLOG("Static: %s, Multiple Shoots: %s", isStatic ? "YES" : "NO", hasMultipleShoots ? "YES" : "NO");*/

    const GameObject* root           = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    const std::vector<UID>& siblings = root->GetChildren();

    /* GLOG("Parent: %s, Children: %d", root->GetName().c_str(), siblings.size());*/

    std::vector<GameObject*> allArrows;

    for (UID objectUID : siblings)
    {
        GameObject* obj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(objectUID);
        if (obj != parent)
        {
            std::string objName = obj->GetName();
            if (objName.find("Arrow_") != std::string::npos)
            {
                /*   GLOG("Found arrow: %s", objName.c_str());*/

                ScriptComponent* scriptComp = obj->GetComponent<ScriptComponent*>();
                if (scriptComp)
                {
                    Projectile* projectile = scriptComp->GetScriptByType<Projectile>();
                    if (projectile)
                    {
                        allArrows.push_back(obj);
                        arrowPool.push_back(projectile);

                        /* GLOG("Arrow added to pool: %s (Total: %d)", objName.c_str(), arrowPool.size());*/
                    }
                }
            }
        }
    }

    /*  GLOG("Total arrows found: %d", allArrows.size());*/

    if (hasMultipleShoots)
    {
        /*   GLOG("=== MULTIPLE SHOOTS ARCHER ===");*/

        for (GameObject* arrowObj : allArrows)
        {
            arrowObj->SetEnabledRecursive(false);
            /* GLOG("Disabled arrow: %s", arrowObj->GetName().c_str());*/
        }
    }
    else
    {
        /* GLOG("=== SINGLE SHOOT ARCHER ===");*/

        if (!allArrows.empty())
        {

            GameObject* singleArrowObj = allArrows[0];
            arrow                      = arrowPool[0];

            /*   GLOG("Single arrow assigned: %s", singleArrowObj->GetName().c_str());*/

            for (GameObject* arrowObj : allArrows)
            {
                arrowObj->SetEnabledRecursive(false);
                /*    GLOG("Disabled arrow: %s", arrowObj->GetName().c_str());*/
            }
        }
        else
        {
            /* GLOG("[WARNING] No arrows found for single shoot archer");*/
        }
    }

    /* GLOG("=== FINAL ARROW STATES ===");*/
    for (int i = 0; i < arrowPool.size(); i++)
    {
        GameObject* arrowObj = arrowPool[i]->GetParent();
        /*  GLOG("Arrow[%d]: %s, Enabled: %s", i, arrowObj->GetName().c_str(), arrowObj->IsEnabled() ? "YES" : "NO");*/
    }
    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] ARCHER: No audio component found");

    /*  GLOG("=== ARCHER INIT COMPLETE ===");*/
    return true;
}

void Archer::Update(float deltaTime)
{
    if (agentAI == nullptr) return;

    if (isKnockback)
    {
        float3 currentPos   = parent->GetGlobalTransform().TranslatePart();

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
    repositionTimer += deltaTime;

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

bool Archer::CheckLineOfSight()
{
    if (!character) return false;

    float3 archerPos                      = parent->GetPosition();
    float3 playerPos                      = character->GetLastPosition();

    Scene* scene                          = AppEngine->GetSceneModule()->GetScene();
    const std::vector<GameObject*>* walls = scene->GetTaggedGameObjects(HashString("Wall"));

    if (!walls || walls->empty()) return true;

    // Simple but effective line of sight check
    for (GameObject* wall : *walls)
    {
        if (!wall->IsEnabled()) continue;

        float3 wallPos           = wall->GetPosition();

        // Check if wall is between archer and player
        float distArcherToPlayer = archerPos.Distance(playerPos);
        float distArcherToWall   = archerPos.Distance(wallPos);
        float distWallToPlayer   = wallPos.Distance(playerPos);

        // If wall is roughly on the line between archer and player
        float totalDist          = distArcherToWall + distWallToPlayer;

        if (abs(totalDist - distArcherToPlayer) < 2.0f && distArcherToWall < distArcherToPlayer &&
            distWallToPlayer < distArcherToPlayer)
        {
            GLOG("Line of sight BLOCKED by wall: %s", wall->GetName().c_str());
            return false;
        }
    }

    GLOG("Line of sight CLEAR");
    return true;
}

bool Archer::ShouldSeekCover()
{
    if (!character || isStatic) return false;

    float distanceToPlayer  = GetDistanceFromPlayer();
    bool hasNearbyAlliesNow = HasNearbyAllies();

    return (distanceToPlayer <= coverSeekRange && !isInCover) || (hasNearbyAlliesNow && !currentCover) ||
           (knockbackTimer > 0.0f);
}

bool Archer::HasNearbyAllies()
{
    Scene* scene                            = AppEngine->GetSceneModule()->GetScene();

    const std::vector<GameObject*>* soldiers = scene->GetTaggedGameObjects(HashString("Soldier"));
    if (soldiers)
    {
        for (GameObject* ally : *soldiers)
        {
            if (ally == parent || !ally->IsEnabled()) continue;

            float distance = parent->GetPosition().Distance(ally->GetPosition());
            if (distance <= allyDetectionRange) return true;
        }
    }

    return false;
}

GameObject* Archer::FindNearestCover()
{
    Scene* scene                          = AppEngine->GetSceneModule()->GetScene();
    const std::vector<GameObject*>* walls = scene->GetTaggedGameObjects(HashString("Wall"));

    if (!walls) return nullptr;

    GameObject* bestCover = nullptr;
    float bestScore       = -1.0f;

    for (GameObject* wall : *walls)
    {
        if (!wall->IsEnabled()) continue;

        float distanceToCover = parent->GetPosition().Distance(wall->GetPosition());
        if (distanceToCover > coverRadius * 4.0f) continue; 

        float score = CalculateCoverScore(wall);
        if (score > bestScore)
        {
            bestScore = score;
            bestCover = wall;
        }
    }

    return bestCover;
}

float Archer::CalculateCoverScore(GameObject* coverObj)
{
    float3 coverPos       = coverObj->GetPosition();
    float distanceToCover = parent->GetPosition().Distance(coverPos);

    float distanceScore   = 1.0f / (1.0f + distanceToCover * 0.1f);

    float protectionScore = 0.0f;
    if (character)
    {
        float3 playerPos         = character->GetLastPosition();
        float3 archerPos         = parent->GetPosition();

        float distArcherToCover  = archerPos.Distance(coverPos);
        float distCoverToPlayer  = coverPos.Distance(playerPos);
        float distArcherToPlayer = archerPos.Distance(playerPos);

        if (abs((distArcherToCover + distCoverToPlayer) - distArcherToPlayer) < 2.0f)
        {
            protectionScore = 1.0f;
        }
    }

    return distanceScore + protectionScore * 2.0f;
}

float3 Archer::FindShootingPosition()
{
    if (!character) return parent->GetPosition();

    // If we don't have cover, just find any clear position
    if (!currentCover)
    {
        GLOG("No cover - finding general clear shooting position");
        return FindClearShootingPosition();
    }

    float3 coverPos  = currentCover->GetPosition();
    float3 playerPos = character->GetLastPosition();

    GLOG("Finding shooting position around cover: %s", currentCover->GetName().c_str());

    // Try positions around the cover that have line of sight to player
    for (int i = 0; i < 16; i++)
    {
        float angle      = (i / 16.0f) * 2.0f * 3.14159f;
        float3 offset    = float3(cos(angle), 0, sin(angle)) * coverRadius;
        float3 candidate = coverPos + offset;

        // Check if this position has line of sight to player
        if (HasLineOfSightFromPosition(candidate, playerPos))
        {
            // Verify position is on navmesh
            if (agentAI)
            {
                bool posOverPoly        = false;
                float3 closestPoint     = float3::zero;
                const float3 searchArea = {1.0f, 2.0f, 1.0f};

                agentAI->GetClosestPointInNavmesh(candidate, searchArea, posOverPoly, closestPoint);
                if (posOverPoly)
                {
                    GLOG("Found shooting position around cover with clear LOS");
                    return closestPoint;
                }
            }
        }
    }

    GLOG("No clear shooting position around cover - using fallback");
    return FindClearShootingPosition();
}

float3 Archer::FindClearShootingPosition()
{
    if (!character) return parent->GetPosition();

    float3 archerPos       = parent->GetPosition();
    float3 playerPos       = character->GetLastPosition();

    // Try positions in a circle around the archer
    const int numPositions = 12;
    const float radius     = 3.0f; // how far to move to find clear shot

    for (int i = 0; i < numPositions; i++)
    {
        float angle         = (i / float(numPositions)) * 2.0f * 3.14159f;
        float3 offset       = float3(cos(angle) * radius, 0, sin(angle) * radius);
        float3 candidatePos = archerPos + offset;

        // Check if this position has line of sight to player
        if (HasLineOfSightFromPosition(candidatePos, playerPos))
        {
            // Make sure the position is on navmesh
            bool posOverPoly        = false;
            float3 closestPoint     = float3::zero;
            const float3 searchArea = {1.0f, 2.0f, 1.0f};

            if (agentAI)
            {
                agentAI->GetClosestPointInNavmesh(candidatePos, searchArea, posOverPoly, closestPoint);
                if (posOverPoly)
                {
                    GLOG("Found clear shooting position");
                    return closestPoint;
                }
            }
        }
    }

    // If no clear position found, try to get closer to player
    float3 directionToPlayer = (playerPos - archerPos).Normalized();
    float3 closerPos         = archerPos + directionToPlayer * 2.0f;

    if (agentAI)
    {
        bool posOverPoly        = false;
        float3 closestPoint     = float3::zero;
        const float3 searchArea = {1.0f, 2.0f, 1.0f};
        agentAI->GetClosestPointInNavmesh(closerPos, searchArea, posOverPoly, closestPoint);
        if (posOverPoly)
        {
            return closestPoint;
        }
    }

    return archerPos; // fallback to current position
}

bool Archer::CanShootSafely()
{
    if (!character || attackCdTimer > 0.0f) return false;

    // FIRST check line of sight - this is critical!
    if (!CheckLineOfSight())
    {
        GLOG("CANNOT SHOOT SAFELY - NO LINE OF SIGHT TO PLAYER");
        return false;
    }

    float distanceToPlayer = GetDistanceFromPlayer();

    // Then check if we're at safe distance or have cover
    bool safeDistance      = (distanceToPlayer >= safeShootingDistance || currentCover != nullptr);

    GLOG("CAN SHOOT SAFELY - LOS: YES, Distance: %.1f, Safe: %s", distanceToPlayer, safeDistance ? "YES" : "NO");
    return safeDistance;
    
}

bool Archer::HasLineOfSightFromPosition(float3 fromPos, float3 toPos)
{
    fromPos.y                             += 1.5f; // eye level
    toPos.y                               += 1.0f; // target center

    Scene* scene                           = AppEngine->GetSceneModule()->GetScene();
    const std::vector<GameObject*>* walls  = scene->GetTaggedGameObjects(HashString("Wall"));

    if (!walls) return true;

    float distance   = fromPos.Distance(toPos);
    float3 direction = (toPos - fromPos).Normalized();

    for (GameObject* wall : *walls)
    {
        if (!wall->IsEnabled()) continue;

        float3 wallPos         = wall->GetPosition();
        float3 toWall          = wallPos - fromPos;
        float projectionLength = toWall.Dot(direction);

        if (projectionLength < 0.5f || projectionLength > distance - 0.5f) continue;

        float3 closestPointOnLine = fromPos + direction * projectionLength;
        float distanceToLine      = wallPos.Distance(closestPointOnLine);

        if (distanceToLine < 1.5f)
        {
            return false;
        }
    }

    return true;
}

void Archer::SeekCover(float deltaTime)
{
    if (!currentCover) return;

    if (animComponent) animComponent->UseTrigger("run");

    float distanceToCover = parent->GetPosition().Distance(coverPosition);
    if (distanceToCover <= coverRadius * 1.5f)
    {
        seekingCover    = false;
        isInCover       = true;
        repositionTimer = 0.0f;
        currentState    = ArcherStates::IN_COVER;
        GLOG("Archer reached cover");
    }
    else
    {
        agentAI->SetPathNavigation(coverPosition);
    }
}

void Archer::StayInCover(float deltaTime)
{
    if (!currentCover || !character) return;

    float distanceToPlayer = GetDistanceFromPlayer();

    GLOG("In cover - Player distance: %.1f, Safe distance: %.1f", distanceToPlayer, safeShootingDistance);

    // If player is at safe distance and we've waited enough, try to find shooting position
    if (distanceToPlayer >= safeShootingDistance && repositionTimer >= repositionDelay)
    {
        // First check if we can shoot from current cover position
        if (CheckLineOfSight() && CanShootSafely())
        {
            GLOG("Can shoot directly from cover position");
            currentState = ArcherStates::AIM;
            isInCover    = false;
            return;
        }

        // Find a shooting position with clear line of sight
        shootingPosition = FindShootingPosition();

        // Verify the shooting position actually has line of sight
        if (HasLineOfSightFromPosition(shootingPosition, character->GetLastPosition()))
        {
            currentState    = ArcherStates::POSITIONING_TO_SHOOT;
            repositionTimer = 0.0f;
            GLOG("Leaving cover to shoot from verified clear position");
        }
        else
        {
            GLOG("Shooting position has no line of sight - staying in cover");
            repositionTimer = repositionDelay * 0.5f; // Try again sooner
        }
    }
    // If player is too close, stay hidden and wait
    else if (distanceToPlayer < safeShootingDistance * 0.8f)
    {
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
        GLOG("Player too close - hiding in cover");
    }
    // If we somehow have clear line of sight from cover, take the shot
    else if (CheckLineOfSight() && CanShootSafely())
    {
        GLOG("Surprise clear shot from cover - taking it!");
        currentState = ArcherStates::AIM;
        isInCover    = false;
    }
}

void Archer::PositionToShoot(float deltaTime)
{
    if (!character) return;

    float distanceToShootPos = parent->GetPosition().Distance(shootingPosition);

    // Check if we've reached the shooting position
    if (distanceToShootPos <= 1.5f)
    {
        GLOG("Reached shooting position - verifying line of sight");

        // Double-check line of sight from this position
        if (CheckLineOfSight() && CanShootSafely())
        {
            GLOG("CLEAR SHOT CONFIRMED - STARTING AIM");
            currentState    = ArcherStates::AIM;
            isInCover       = false;
            repositionTimer = 0.0f;
        }
        else
        {
            GLOG("NO CLEAR SHOT FROM POSITION - FINDING NEW POSITION");

            // Try to find a better position
            float3 newShootingPos = FindClearShootingPosition();
            float distanceToNew   = parent->GetPosition().Distance(newShootingPos);

            if (distanceToNew > 1.0f && distanceToNew < 10.0f) // reasonable distance
            {
                shootingPosition = newShootingPos;
                repositionTimer  = 0.0f;
                GLOG("Moving to new shooting position");
            }
            else
            {
                // Can't find good shooting position, try chasing instead
                GLOG("Cannot find clear shooting position - switching to CHASE");
                currentState    = ArcherStates::CHASE;
                repositionTimer = 0.0f;
            }
        }
    }
    else
    {
        // Continue moving to shooting position
        agentAI->SetPathNavigation(shootingPosition);
        if (animComponent) animComponent->UseTrigger("run");

        GLOG("Moving to shooting position (%.1f units away)", distanceToShootPos);
    }

    // Timeout protection - don't get stuck trying to reach impossible positions
    if (repositionTimer >= 5.0f)
    {
        GLOG("POSITIONING TIMEOUT - SWITCHING TO CHASE");
        currentState    = ArcherStates::CHASE;
        repositionTimer = 0.0f;
    }
}

void Archer::OnPlayerExitLocation()
{
    currentState = ArcherStates::PATROL;
    agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
    seekingCover       = false;
    isInCover          = false;
    currentCover       = nullptr;
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
    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        hasShot            = false;
        isAttacking        = false;
        hasStartedShooting = false;
        currentShot        = 0;
        shotTimer          = 0.0f;
        attackCdTimer      = attackCooldown;
        agentAI->ResetSpeed();
        agentAI->SetLookForward(true);
        isAiming     = false;
        aimTimer     = 0.0f;
        currentState = ArcherStates::PATROL;
        return;
    }

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

                float3 predictedTarget = CalculatePredictiveTarget();
                float3 baseDirection   = (predictedTarget - parent->GetGlobalTransform().TranslatePart()).Normalized();

                float spreadAngle      = 10.0f * (3.14159f / 180.0f);
                float randomAngle      = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngle;
                float3 shootDirection  = baseDirection;
                float cosA             = std::cos(randomAngle);
                float sinA             = std::sin(randomAngle);
                float x                = shootDirection.x * cosA - shootDirection.z * sinA;
                float z                = shootDirection.x * sinA + shootDirection.z * cosA;
                shootDirection.x       = x;
                shootDirection.z       = z;

                float3 arrowPos        = float3(parent->GetPosition().x, 1.3f, parent->GetPosition().z);

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
                    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_OVERSHOOTING);
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
    case ArcherStates::SEEKING_COVER:
        SeekCover(deltaTime);
        break;
    case ArcherStates::IN_COVER:
        StayInCover(deltaTime);
        break;
    case ArcherStates::POSITIONING_TO_SHOOT:
        PositionToShoot(deltaTime);
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
    if (isDead) return;

    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        GLOG("PLAYER IS DEAD - GOING TO PATROL");
        currentState    = ArcherStates::PATROL;
        isRepositioning = false;
        repositionTimer = 0.0f;
        seekingCover    = false;
        isInCover       = false;
        currentCover    = nullptr;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    hasLineOfSight       = CheckLineOfSight();

    // Don't change states while repositioning
    if (isRepositioning)
    {
        return;
    }

    // Static archers shouldn't move much
    if (isStatic)
    {
        if (distance <= maxDetectionRange)
        {
            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f && hasLineOfSight) currentState = ArcherStates::AIM;
            else if (distance <= rangeAIAttack && attackCdTimer > 0.0f) currentState = ArcherStates::SEARCH;
            else if (distance <= rangeAIAttack && !hasLineOfSight)
            {
                // Static archer without line of sight should try minor repositioning
                GLOG("STATIC ARCHER WITHOUT LOS - MINOR REPOSITIONING");
                isRepositioning  = true;
                repositionTimer  = 0.0f;
                repositionTarget = FindClearShootingPosition();
            }
            else currentState = ArcherStates::AIM;
        }
        else
        {
            currentState = ArcherStates::SEARCH;
        }
        return;
    }

    // For MOBILE archers (isStatic = false), handle cover states first
    if (currentState == ArcherStates::SEEKING_COVER || currentState == ArcherStates::IN_COVER ||
        currentState == ArcherStates::POSITIONING_TO_SHOOT)
    {
        // Only escape if VERY close
        if (distance <= rangeEscape)
        {
            GLOG("PLAYER TOO CLOSE - ESCAPING FROM COVER");
            currentState = ArcherStates::ESCAPE;
            isInCover    = false;
            seekingCover = false;
            currentCover = nullptr;
        }
        return; // Stay in cover states otherwise
    }

    // MAIN LOGIC: Try cover BEFORE escape for mobile archers

    // 1. If player is close but not too close, seek cover first
    if (distance <= coverSeekRange && distance > rangeEscape)
    {
        GLOG("PLAYER IN COVER RANGE - SEEKING COVER FIRST");
        currentCover = FindNearestCover();
        if (currentCover)
        {
            seekingCover    = true;
            isInCover       = false;
            coverPosition   = currentCover->GetPosition();
            currentState    = ArcherStates::SEEKING_COVER;
            repositionTimer = 0.0f;
            GLOG("Archer seeking cover at wall: %s", currentCover->GetName().c_str());
            return;
        }
        else
        {
            GLOG("NO COVER FOUND - WILL HANDLE NORMALLY");
        }
    }

    // 2. Only escape if player is VERY close AND no cover is available
    if (distance <= rangeEscape)
    {
        // Try to find cover first even when escaping
        if (!currentCover)
        {
            currentCover = FindNearestCover();
        }

        if (currentCover && distance > rangeEscape * 0.7f) // If we have some room
        {
            GLOG("CLOSE PLAYER BUT COVER AVAILABLE - SEEKING COVER INSTEAD OF ESCAPE");
            seekingCover    = true;
            isInCover       = false;
            coverPosition   = currentCover->GetPosition();
            currentState    = ArcherStates::SEEKING_COVER;
            repositionTimer = 0.0f;
        }
        else
        {
            GLOG("PLAYER TOO CLOSE - ESCAPING (no cover or too close)");
            currentState = ArcherStates::ESCAPE;
            isInCover    = false;
            seekingCover = false;
            currentCover = nullptr;
        }
        return;
    }

    // 3. Normal combat logic
    if (distance <= rangeAIAttack && hasLineOfSight)
    {
        currentState = ArcherStates::AIM;
    }
    else if (distance <= rangeAIAttack && !hasLineOfSight)
    {
        // In attack range but no line of sight - chase to reposition
        GLOG("IN ATTACK RANGE BUT NO LOS - CHASING TO REPOSITION");
        currentState = ArcherStates::CHASE;
    }
    else if (distance <= rangeAIChase)
    {
        currentState = ArcherStates::CHASE;
    }
    else if (distance > maxDetectionRange)
    {
        currentState = ArcherStates::SEARCH;
    }
    else
    {
        currentState = ArcherStates::PATROL;
    }
}

void Archer::SearchForPlayer()
{

    if (isStatic)
    {

        if (animComponent) animComponent->UseTrigger("aim");

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
        if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
        {
            isSearching  = false;
            currentState = ArcherStates::PATROL;
            agentAI->ResetSpeed();
            return;
        }
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
    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        isAiming = false;
        aimTimer = 0.0f;
        agentAI->SetLookForward(true);
        agentAI->ResetSpeed();
        currentState = ArcherStates::PATROL;
        return;
    }

    if (!weaponCollider) return;

    // Check line of sight FIRST
    if (!CheckLineOfSight())
    {
        GLOG("AIM -> Lost line of sight, switching to CHASE");
        isAiming     = false;
        aimTimer     = 0.0f;
        currentState = ArcherStates::CHASE;
        return;
    }

    if (!isAiming)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("aim");
        agentAI->SetSpeed(0.0f, 0.0f);
        isAiming = true;
        aimTimer = 0.0f;
        GLOG("ARCHER STARTED AIMING");
    }
    else
    {
        aimTimer += deltaTime;

        if (character)
        {
            float3 predictedTarget = CalculatePredictiveTarget();
            agentAI->LookAtMovement(predictedTarget, deltaTime);
        }

        if (aimTimer >= aimDuration)
        {
            // Double-check line of sight before shooting
            if (CheckLineOfSight())
            {
                GLOG("ARCHER FINISHED AIMING - ATTACKING");
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
            else
            {
                GLOG("Lost LOS during aim - switching to CHASE");
                isAiming     = false;
                aimTimer     = 0.0f;
                currentState = ArcherStates::CHASE;
            }
        }
    }
}

float3 Archer::CalculatePredictiveTarget()
{
    if (!character) return float3::zero;

    float3 playerPos      = character->GetLastPosition();
    float3 playerVelocity = character->GetVelocity();

    if (!character->IsMoving())
    {
        return playerPos; 
    }

    float arrowSpeed       = 15.0f;
    float distanceToPlayer = (playerPos - parent->GetGlobalTransform().TranslatePart()).Length();
    float timeToReach      = distanceToPlayer / arrowSpeed;

    float3 predictedPos    = character->GetPredictedPosition(timeToReach);

    float accuracy         = 0.85f; 
    float3 inaccuracy      = float3(
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * (1.0f - accuracy), 0.0f,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * (1.0f - accuracy)
    );

    return predictedPos + inaccuracy;
}

void Archer::Attack(float deltaTime)
{
    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        hasShot     = false;
        isAttacking = false;
        agentAI->ResetSpeed();
        agentAI->SetLookForward(true);
        isAiming     = false;
        aimTimer     = 0.0f;
        currentState = ArcherStates::PATROL;
        return;
    }

    // Check line of sight before attacking
    if (!CheckLineOfSight())
    {
        GLOG("ATTACK -> Lost line of sight, switching to CHASE");
        hasShot      = false;
        isAttacking  = false;
        currentState = ArcherStates::CHASE;
        return;
    }

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
        float3 predictedTarget = CalculatePredictiveTarget();
        agentAI->LookAtMovement(predictedTarget, deltaTime);

        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            // Final line of sight check before shooting
            if (CheckLineOfSight())
            {
                hasShot = true;
                if (!arrow) return;

                float3 predictedTarget = CalculatePredictiveTarget();
                float3 direction       = (predictedTarget - parent->GetGlobalTransform().TranslatePart()).Normalized();
                float3 arrowPos        = float3(parent->GetPosition().x, 1.3f, parent->GetPosition().z);

                GameObject* arrowObj   = arrow->GetParent();
                if (arrowObj)
                {
                    arrowObj->SetEnabled(true);
                    arrowObj->SetEnabledRecursive(true);
                    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_ATTACK);
                    arrow->Shoot(arrowPos, direction);
                    GLOG("ARROW SHOT WITH CLEAR LOS");
                }
            }
            else
            {
                GLOG("ATTACK -> No LOS at shot time, switching to CHASE");
                hasShot      = false;
                isAttacking  = false;
                currentState = ArcherStates::CHASE;
                return;
            }
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
        seekingCover = false;
        isInCover    = false;
        currentCover = nullptr;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    bool hasLOS          = CheckLineOfSight();

    GLOG("ChangeState - Distance: %.1f, LOS: %s, Static: %s", distance, hasLOS ? "YES" : "NO", isStatic ? "YES" : "NO");

    // Static archers - simplified logic
    if (isStatic)
    {
        if (distance <= rangeAIAttack && hasLOS && attackCdTimer <= 0.0f)
        {
            currentState = ArcherStates::AIM;
        }
        else
        {
            currentState = ArcherStates::SEARCH;
        }
        return;
    }

    // Mobile archers logic
    if (distance <= rangeEscape)
    {
        GLOG("Player very close - ESCAPING");
        currentState = ArcherStates::ESCAPE;
        seekingCover = false;
        isInCover    = false;
        currentCover = nullptr;
    }
    else if (distance <= rangeAIAttack)
    {
        if (hasLOS)
        {
            GLOG("In attack range with LOS - AIMING");
            currentState = ArcherStates::AIM;
        }
        else
        {
            GLOG("In attack range but no LOS - CHASING");
            currentState = ArcherStates::CHASE;
        }
    }
    else if (distance <= rangeAIChase)
    {
        GLOG("In chase range - CHASING");
        currentState = ArcherStates::CHASE;
    }
    else if (distance > maxDetectionRange)
    {
        GLOG("Player far away - SEARCHING");
        currentState = ArcherStates::SEARCH;
    }
    else
    {
        GLOG("Default - PATROL");
        currentState = ArcherStates::PATROL;
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