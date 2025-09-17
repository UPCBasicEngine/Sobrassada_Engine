#include "pch.h"

#include "Application.h"
#include "Archer.h"
#include "ArcherProjectile.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Geometry/LineSegment.h"
#include "Globals.h"
#include "ParticleSystemComponent.h"
#include "RaycastController.h"
#include "ResourceStateMachine.h"
#include "ScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
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
    scene           = AppEngine->GetSceneModule()->GetScene();
    walls           = scene->GetTaggedGameObjects(HashString("Wall"));
    soldiers        = scene->GetTaggedGameObjects(HashString("Soldier"));
    archerVfxObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(archerHitVFX);
    if (!archerVfxObject) GLOG("[WARNING] No melee VFX found for melee attack in Archer")
    else archerVfxObject->SetEnabled(false);
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

    const GameObject* root           = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    const std::vector<UID>& siblings = root->GetChildren();

    std::vector<GameObject*> allArrows;

    for (UID objectUID : siblings)
    {
        GameObject* obj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(objectUID);
        if (obj != parent)
        {
            std::string objName = obj->GetName();
            if (objName.find("Arrow_") != std::string::npos)
            {

                ScriptComponent* scriptComp = obj->GetComponent<ScriptComponent*>();
                if (scriptComp)
                {
                    ArcherProjectile* projectile = scriptComp->GetScriptByType<ArcherProjectile>();
                    if (projectile)
                    {
                        allArrows.push_back(obj);
                        arrowPool.push_back(projectile);
                    }
                }
            }
        }
    }

    if (hasMultipleShoots)
    {
        for (GameObject* arrowObj : allArrows)
        {
            arrowObj->SetEnabledRecursive(false);
        }
    }
    else
    {

        if (!allArrows.empty())
        {

            GameObject* singleArrowObj = allArrows[0];
            arrow                      = arrowPool[0];

            for (GameObject* arrowObj : allArrows)
            {
                arrowObj->SetEnabledRecursive(false);
            }
        }
        else GLOG("[WARNING] No arrows found for single shoot archer");
    }

    for (int i = 0; i < arrowPool.size(); i++)
    {
        GameObject* arrowObj = arrowPool[i]->GetParent();
    }

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] Archer: No audio component found");
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
    if (hitVfxIsActive && archerVfxObject->IsEnabled())
    {
        hitVfxTimer += deltaTime;
        if (hitVfxTimer >= hitVfxDuration)
        {
            archerVfxObject->SetEnabled(false);
            hitVfxTimer    = 0.0f;
            hitVfxIsActive = false;
        }
    }
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
        const std::string life       = "Health: " + std::to_string(currentHealth);
        const std::string animState  = "Anim state: " + stateName.GetString();
        const std::string logicState = "Logic state: " + GetLogicStateName();

        std::vector<std::pair<std::string, float2>> logs {
            {life,       float2(-50.0f, -140.0f)},
            {animState,  float2(-80.0f, -160.0f)},
            {logicState, float2(-80.0f, -180.0f)},
        };

        if (currentState == ArcherStates::SEEKING_COVER && currentCover)
        {
            const std::string coverInfo =
                "Cover Target: (" + std::to_string(coverPosition.x) + ", " + std::to_string(coverPosition.z) + ")";

            logs.push_back({coverInfo, float2(-100.0f, -200.0f)});
        }

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

bool Archer::CheckLineOfSight()
{
    if (!character) return false;

    float3 archerPos  = parent->GetPosition();
    float3 playerPos  = character->GetLastPosition();

    archerPos.y      += 1.5f;
    playerPos.y      += 1.0f;

    LineSegment sightRay(archerPos, playerPos);

    std::vector<GameObject*> potentialBlockers;

    if (walls && !walls->empty())
    {
        for (GameObject* wall : *walls)
        {
            if (wall && wall->IsEnabled())
            {
                potentialBlockers.push_back(wall);
            }
        }
    }

    GameObject* hitObject = RaycastController::GetRayIntersectionObject(sightRay, potentialBlockers);

    if (hitObject != nullptr)
    {
        GLOG("Line of sight BLOCKED by: %s", hitObject->GetName().c_str());
        return false;
    }

    return true;
}

bool Archer::ShouldSeekCover()
{
    if (!character || isStatic || seekingCover || isInCover) return false;

    float distanceToPlayer = GetDistanceFromPlayer();

    if (distanceToPlayer <= rangeEscape * 1.2f)
    {
        return false;
    }

    static float lastCoverSeekTime = 0.0f;
    float currentTime              = AppEngine->GetGameTimer()->GetTime();

    if (currentTime - lastCoverSeekTime < 5.0f)
    {
        return false;
    }

    GameObject* nearestCover = FindNearestCover();
    if (!nearestCover)
    {
        GLOG("ShouldSeekCover: No valid cover available");
        return false;
    }

    bool shouldSeek =
        (distanceToPlayer > rangeEscape * 1.5f && distanceToPlayer <= coverSeekRange) || (knockbackTimer > 0.0f);

    if (shouldSeek)
    {
        lastCoverSeekTime = currentTime;
        GLOG("ShouldSeekCover: YES - Distance: %.1f", distanceToPlayer);
    }

    return shouldSeek;
}

bool Archer::HasNearbyAllies()
{
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

    if (!walls || !character) return nullptr;

    GameObject* bestCover = nullptr;
    float bestScore       = -1.0f;
    float3 playerPos      = character->GetLastPosition();
    float3 archerPos      = parent->GetPosition();

    GLOG("FindNearestCover: Checking %d walls", (int)walls->size());

    for (GameObject* wall : *walls)
    {
        if (!wall || !wall->IsEnabled()) continue;

        float3 wallPos       = wall->GetPosition();
        float distanceToWall = archerPos.Distance(wallPos);

        if (distanceToWall > 25.0f) continue;

        float wallToPlayerDistance = wallPos.Distance(playerPos);
        if (wallToPlayerDistance < rangeEscape * 1.5f)
        {
            GLOG("Wall %s too close to player (%.1f) - skipping", wall->GetName().c_str(), wallToPlayerDistance);
            continue;
        }

        float3 archerToPlayer = playerPos - archerPos;
        float3 archerToWall   = wallPos - archerPos;

        float playerDistance  = archerToPlayer.Length();
        float wallDistance    = archerToWall.Length();

        float distanceScore   = 1.0f / (1.0f + distanceToWall * 0.1f);

        float coverageScore   = 0.0f;
        if (wallDistance < playerDistance)
        {
            float3 wallDirection   = archerToWall.Normalized();
            float3 playerDirection = archerToPlayer.Normalized();
            float alignment        = wallDirection.Dot(playerDirection);

            if (alignment > 0.3f)
            {
                coverageScore = alignment * 2.0f;
            }
        }

        float playerDistanceBonus = wallToPlayerDistance > rangeEscape * 2.0f ? 1.0f : 0.0f;

        float totalScore          = distanceScore + coverageScore + playerDistanceBonus;

        GLOG(
            "Wall %s: WallDist=%.1f, PlayerToWall=%.1f, Coverage=%.2f, Total=%.2f", wall->GetName().c_str(),
            wallDistance, wallToPlayerDistance, coverageScore, totalScore
        );

        if (totalScore > bestScore)
        {
            bestScore = totalScore;
            bestCover = wall;
        }
    }

    if (bestCover)
    {
        float3 selectedWallPos = bestCover->GetPosition();
        float distanceToPlayer = selectedWallPos.Distance(playerPos);
        GLOG("Selected cover: %s (%.1f units from player)", bestCover->GetName().c_str(), distanceToPlayer);
    }
    else
    {
        GLOG("No suitable cover found - all walls too close to player or no coverage");
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

        if (abs((distArcherToCover + distCoverToPlayer) - distArcherToPlayer) < 2.0f) protectionScore = 1.0f;
    }

    return distanceScore + protectionScore * 2.0f;
}

float3 Archer::FindShootingPosition()
{
    if (!currentCover || !character || !agentAI) return parent->GetPosition();

    float3 coverPos          = currentCover->GetPosition();
    float3 playerPos         = character->GetLastPosition();
    float3 currentPos        = parent->GetPosition();
    const float3 searchArea  = {2.0f, 2.0f, 2.0f};

    std::vector<float> radii = {coverRadius * 1.5f, coverRadius * 2.0f, coverRadius * 2.5f};

    for (float radius : radii)
    {
        for (int i = 0; i < 8; i++)
        {
            float angle         = (i / 8.0f) * 2.0f * 3.14159f;
            float3 offset       = float3(cos(angle), 0, sin(angle)) * radius;
            float3 candidate    = coverPos + offset;

            bool posOverPoly    = false;
            float3 closestPoint = float3::zero;
            agentAI->GetClosestPointInNavmesh(candidate, searchArea, posOverPoly, closestPoint);

            if (posOverPoly && closestPoint.Distance(currentPos) > 1.0f &&
                HasLineOfSightFromPosition(closestPoint, playerPos))
            {
                GLOG("Found shooting position at distance %.2f from current", closestPoint.Distance(currentPos));
                return closestPoint;
            }
        }
    }

    float3 dirToPlayer  = (playerPos - currentPos).Normalized();
    float3 forwardPos   = currentPos + dirToPlayer * 2.0f;

    bool posOverPoly    = false;
    float3 closestPoint = float3::zero;
    agentAI->GetClosestPointInNavmesh(forwardPos, searchArea, posOverPoly, closestPoint);

    if (posOverPoly && closestPoint.Distance(currentPos) > 1.0f)
    {
        GLOG("Using forward position as fallback");
        return closestPoint;
    }

    return currentPos;
}

float3 Archer::FindClearShootingPosition()
{
    if (!character || !agentAI) return parent->GetPosition();

    float3 archerPos        = parent->GetPosition();
    float3 playerPos        = character->GetLastPosition();

    const int numPositions  = 12;
    const float radius      = 3.0f;
    const float3 searchArea = {2.0f, 2.0f, 2.0f};

    for (int i = 0; i < numPositions; i++)
    {
        float angle         = (i / float(numPositions)) * 2.0f * 3.14159f;
        float3 offset       = float3(cos(angle) * radius, 0, sin(angle) * radius);
        float3 candidatePos = archerPos + offset;

        bool posOverPoly    = false;
        float3 closestPoint = float3::zero;
        agentAI->GetClosestPointInNavmesh(candidatePos, searchArea, posOverPoly, closestPoint);

        if (posOverPoly && HasLineOfSightFromPosition(closestPoint, playerPos))
        {
            GLOG("Found clear shooting position");
            return closestPoint;
        }
    }

    return archerPos;
}

bool Archer::CanShootSafely()
{
    if (!character || attackCdTimer > 0.0f) return false;

    float distanceToPlayer = GetDistanceFromPlayer();
    hasLineOfSight         = CheckLineOfSight();

    return (distanceToPlayer >= safeShootingDistance || (currentCover != nullptr)) && hasLineOfSight;
}

bool Archer::HasLineOfSightFromPosition(float3 fromPos, float3 toPos)
{

    fromPos.y += 1.5f;
    toPos.y   += 1.0f;

    LineSegment sightRay(fromPos, toPos);

    if (!walls || walls->empty()) return true;

    std::vector<GameObject*> potentialBlockers;
    for (GameObject* wall : *walls)
    {
        if (wall && wall->IsEnabled())
        {
            potentialBlockers.push_back(wall);
        }
    }

    GameObject* hitObject = RaycastController::GetRayIntersectionObject(sightRay, potentialBlockers);

    return (hitObject == nullptr);
}

void Archer::SeekCover(float deltaTime)
{
    if (!currentCover || !agentAI)
    {
        seekingCover = false;
        currentState = ArcherStates::CHASE;
        return;
    }

    float3 currentPos = parent->GetPosition();
    float3 wallPos    = currentCover->GetPosition();

    float3 targetPos;
    bool foundValidPos       = false;

    const float searchRadius = 3.0f;
    const int numPoints      = 8;

    for (int i = 0; i < numPoints && !foundValidPos; i++)
    {
        float angle         = (i / float(numPoints)) * 2.0f * 3.14159f;
        float3 offset       = float3(cos(angle) * searchRadius, 0.0f, sin(angle) * searchRadius);
        float3 candidate    = wallPos + offset;

        bool posOverPoly    = false;
        float3 closestPoint = float3::zero;
        float3 searchArea   = {2.0f, 2.0f, 2.0f};

        agentAI->GetClosestPointInNavmesh(candidate, searchArea, posOverPoly, closestPoint);

        if (posOverPoly)
        {
            targetPos     = closestPoint;
            foundValidPos = true;
            GLOG("Found navigable position near wall at (%.1f, %.1f, %.1f)", targetPos.x, targetPos.y, targetPos.z);
        }
    }

    if (!foundValidPos)
    {
        bool posOverPoly    = false;
        float3 closestPoint = float3::zero;
        float3 searchArea   = {5.0f, 2.0f, 5.0f};

        agentAI->GetClosestPointInNavmesh(wallPos, searchArea, posOverPoly, closestPoint);

        if (posOverPoly)
        {
            targetPos     = closestPoint;
            foundValidPos = true;
            GLOG("Using closest navigable point to wall at (%.1f, %.1f, %.1f)", targetPos.x, targetPos.y, targetPos.z);
        }
        else
        {
            GLOG("ERROR: No navigable position found near wall - switching to chase");
            seekingCover = false;
            currentState = ArcherStates::CHASE;
            return;
        }
    }

    if (animComponent) animComponent->UseTrigger("run");

    bool pathResult = agentAI->SetPathNavigation(targetPos);
    GLOG("Navigation to cover position: %s", pathResult ? "SUCCESS" : "FAILED");

    if (!pathResult)
    {
        GLOG("Navigation failed - trying manual movement");
        seekingCover = false;
        currentState = ArcherStates::CHASE;
        return;
    }

    float distanceToTarget = currentPos.Distance(targetPos);
    GLOG("SeekCover: Distance to target: %.2f", distanceToTarget);

    if (distanceToTarget <= 2.0f)
    {
        GLOG("Reached cover area successfully");
        seekingCover    = false;
        isInCover       = true;
        repositionTimer = 0.0f;
        timeInCover     = 0.0f;
        currentState    = ArcherStates::IN_COVER;
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
    }

    float seekStartTime = 0.0f;
    float currentTime   = AppEngine->GetGameTimer()->GetTime();
    if (seekStartTime == 0.0f) seekStartTime = currentTime;

    if (currentTime - seekStartTime > 8.0f)
    {
        GLOG("SeekCover timeout - switching to chase");
        seekingCover  = false;
        seekStartTime = 0.0f;
        currentState  = ArcherStates::CHASE;
    }
}

void Archer::StayInCover(float deltaTime)
{
    if (!currentCover || !character) return;

    float distanceToPlayer  = GetDistanceFromPlayer();
    timeInCover            += deltaTime;

    GLOG(
        "StayInCover: Distance=%.1f, TimeInCover=%.1f, RepositionTimer=%.1f", distanceToPlayer, timeInCover,
        repositionTimer
    );

    if (distanceToPlayer >= safeShootingDistance && repositionTimer >= repositionDelay && timeInCover >= 1.0f)
    {
        shootingPosition            = FindShootingPosition();
        float3 currentPos           = parent->GetPosition();

        float distanceToShootingPos = currentPos.Distance(shootingPosition);

        if (distanceToShootingPos > 0.5f && HasLineOfSightFromPosition(shootingPosition, character->GetLastPosition()))
        {
            currentState    = ArcherStates::POSITIONING_TO_SHOOT;
            repositionTimer = 0.0f;
            timeInCover     = 0.0f;
            GLOG("Archer leaving cover to shoot at distance %.2f", distanceToShootingPos);
        }
        else
        {
            if (hasLineOfSight && CanShootSafely())
            {
                currentState    = ArcherStates::AIM;
                isInCover       = false;
                repositionTimer = 0.0f;
                timeInCover     = 0.0f;
                GLOG("Archer leaving cover to aim directly");
            }
            else
            {
                repositionTimer = 0.0f;
                GLOG("No valid shooting position found, staying in cover");
            }
        }
    }
    else if (distanceToPlayer < safeShootingDistance * 0.8f)
    {
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
    }

    if (timeInCover >= 8.0f)
    {
        GLOG("Cover timeout - forcing exit");
        timeInCover  = 0.0f;
        isInCover    = false;
        currentCover = nullptr;
        currentState = ArcherStates::CHASE;
    }
}

void Archer::PositionToShoot(float deltaTime)
{
    float distanceToShootPos = parent->GetPosition().Distance(shootingPosition);
    float3 lastArcherPos     = parent->GetPosition();
    if (distanceToShootPos <= 1.0f)
    {
        if (CheckLineOfSight() && CanShootSafely())
        {
            currentState    = ArcherStates::AIM;
            isInCover       = false;
            repositionTimer = 0.0f;
        }
        else
        {

            shootingPosition = FindClearShootingPosition();
            if (shootingPosition.x == lastArcherPos.x && shootingPosition.y == lastArcherPos.y &&
                shootingPosition.z == lastArcherPos.z)
            {
                currentState    = ArcherStates::IN_COVER;
                repositionTimer = 0.0f;
            }
        }
    }
    else
    {
        agentAI->SetPathNavigation(shootingPosition);
        if (animComponent) animComponent->UseTrigger("run");
    }

    if (repositionTimer >= 5.0f)
    {
        GLOG("Positioning timeout - forcing state change");
        currentState    = ArcherStates::CHASE;
        repositionTimer = 0.0f;
    }
}

const std::string Archer::GetLogicStateName()
{
    switch (currentState)
    {
    case ArcherStates::PATROL:
        return "Idle";
        break;
    case ArcherStates::CHASE:
        return "Run";
        break;
    case ArcherStates::SEEKING_COVER:
        return "Seek Cover";
        break;
    case ArcherStates::BASIC_ATTACK:
        return "Basic Attack";
        break;
    case ArcherStates::AIM:
        return "Aiming";
        break;
    case ArcherStates::OVERSHOOTING:
        return "Overshooting";
        break;
    case ArcherStates::IN_COVER:
        return "Reached Cover and in cover";
        break;
    case ArcherStates::POSITIONING_TO_SHOOT:
        return "Looking Position to shoot";
        break;
    case ArcherStates::SEARCH:
        return "Search Player";
        break;
    case ArcherStates::ESCAPE:
        return "Archer Escaping";
        break;
    case ArcherStates::DEATH:
        return "Death";
        break;
    default:
        return "MISSING!";
        break;
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
    isAttacking    = false;
    attackTimer    = 0.0f;
    isAiming       = false;
    aimTimer       = 0.0f;
    hitVfxIsActive = true;

    if (weaponCollider && weaponCollider->GetEnabled())
    {
        weaponCollider->SetEnabled(false);
    }

    isKnockback    = true;
    knockbackTimer = knockbackTime;
    ApplyKnockback();

    if (hitVfxIsActive && archerVfxObject && !archerVfxObject->IsEnabled())
    {
        GLOG("Activating arrow VFX - isActive: %s, timer: %f", hitVfxIsActive ? "true" : "false", hitVfxTimer);

        archerVfxObject->SetEnabled(true);

        ParticleSystemComponent* particleSystem = archerVfxObject->GetComponent<ParticleSystemComponent*>();
        if (particleSystem)
        {
            particleSystem->SpawnAllInstances();
            GLOG("Arrow VFX particles spawned");
        }
    }
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

                ArcherProjectile* currentArrow = arrowPool[currentArrowIndex];
                GameObject* arrowGameObject    = currentArrow->GetParent();

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
                else GLOG("[ERROR] Arrow GameObject is NULL!");

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

            if (!isStatic) ChangeState();
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

    if (!playerScript->IsDead() && playerScript->GetState() != CharacterStates::RESPAWN)
    {
        if (animComponent) animComponent->UseTrigger("idle");
        float distance = GetDistanceFromPlayer();

        GLOG("PATROL - Player distance: %.1f", distance);

        if (distance <= rangeAIChase)
        {
            GLOG("PATROL -> CHASE (player in chase range)");
            currentState = ArcherStates::CHASE;
            return;
        }
        else if (distance <= maxDetectionRange)
        {
            GLOG("PATROL -> SEARCH (player detected)");
            currentState = ArcherStates::SEARCH;
            return;
        }
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
    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        currentState = ArcherStates::PATROL;
        return;
    }

    if (animComponent) animComponent->UseTrigger("run");

    if (character != nullptr)
    {
        agentAI->SetPathNavigation(character->GetLastPosition());
        agentAI->SetLookForward(true);
        ChangeState();
    }
    else currentState = ArcherStates::PATROL;
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

            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f) currentState = ArcherStates::AIM;
        }
        else if (agentAI) agentAI->SetSpeed(0.0f, 0.0f);
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
    GLOG("ENTER AIM STATE");
    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        isAiming = false;
        aimTimer = 0.0f;
        agentAI->SetLookForward(true);
        agentAI->ResetSpeed();
        currentState = ArcherStates::PATROL;
        return;
    }

    float distance = GetDistanceFromPlayer();
    if (!weaponCollider) return;

    if (!CheckLineOfSight() && !isStatic)
    {
        GLOG("ARCHER LOST LINE OF SIGHT - GOING TO CHASE");
        isAiming     = false;
        aimTimer     = 0.0f;
        currentState = ArcherStates::CHASE;
        return;
    }

    if (!isAiming)
    {
        GLOG("PREPARING TO AIM");
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("aim");

        isAiming = true;
        aimTimer = 0.0f;
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
            isAiming = false;
            aimTimer = 0.0f;

            if (hasMultipleShoots) currentState = ArcherStates::OVERSHOOTING;
            else currentState = ArcherStates::BASIC_ATTACK;
        }
    }
}

float3 Archer::CalculatePredictiveTarget()
{
    if (!character) return float3::zero;

    float3 playerPos      = character->GetLastPosition();
    float3 playerVelocity = character->GetVelocity();

    if (!character->IsMoving()) return playerPos;

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
        float3 predictedTarget = CalculatePredictiveTarget();
        agentAI->LookAtMovement(predictedTarget, deltaTime);

        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            if (!CheckLineOfSight())
            {
                GLOG("ATTACK -> No LOS at shot time, switching to CHASE");
                hasShot      = false;
                isAttacking  = false;
                currentState = ArcherStates::CHASE;
                return;
            }
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
        if (currentState != ArcherStates::PATROL)
        {
            GLOG("Player dead - switching to patrol");
            currentState = ArcherStates::PATROL;
            seekingCover = false;
            isInCover    = false;
            currentCover = nullptr;
            timeInCover  = 0.0f;
        }
        return;
    }

    const float distance = GetDistanceFromPlayer();
    hasLineOfSight       = CheckLineOfSight();

    if (isStatic)
    {
        if (distance <= maxDetectionRange)
        {
            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f) currentState = ArcherStates::AIM;
            else if (distance <= rangeAIAttack && attackCdTimer > 0.0f) currentState = ArcherStates::SEARCH;
            else currentState = ArcherStates::AIM;
        }
        else currentState = ArcherStates::SEARCH;

        return;
    }

    if (currentState == ArcherStates::SEEKING_COVER || currentState == ArcherStates::IN_COVER ||
        currentState == ArcherStates::POSITIONING_TO_SHOOT)
    {
        if (distance <= rangeEscape)
        {
            currentState = ArcherStates::ESCAPE;
            isInCover    = false;
            seekingCover = false;
            currentCover = nullptr;
        }
        return;
    }

    if (distance <= rangeEscape)
    {
        currentState = ArcherStates::ESCAPE;
        isInCover    = false;
        seekingCover = false;
        currentCover = nullptr;
    }

    else if (ShouldSeekCover())
    {
        currentCover = FindNearestCover();
        if (currentCover)
        {
            seekingCover = true;
            isInCover    = false;
            timeInCover  = 0.0f;
            currentState = ArcherStates::SEEKING_COVER;
            GLOG("Starting cover seek for wall: %s", currentCover->GetName().c_str());
        }
        else
        {

            if (distance < rangeAIAttack && hasLineOfSight) currentState = ArcherStates::AIM;
            else currentState = ArcherStates::CHASE;
        }
    }

    else if (distance < rangeAIAttack && hasLineOfSight) currentState = ArcherStates::AIM;
    else if (distance >= rangeAIChase) currentState = ArcherStates::CHASE;
    else if (distance > maxDetectionRange) currentState = ArcherStates::SEARCH;
    else currentState = ArcherStates::PATROL;
}

void Archer::Escape(float deltaTime)
{
    GLOG("ESCAPE STATE");
    if (!agentAI || !character) return;

    float3 archerPos        = parent->GetGlobalTransform().TranslatePart();
    const float3 searchArea = {1.0f, 2.0f, 1.0f};
    bool posOverPoly        = false;
    float3 closestPoint     = float3::zero;
    if (animComponent) animComponent->UseTrigger("run");
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

                if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) >= rangeEscape)
                {
                    hasEscapeTarget = false;
                    ChangeState();
                }
                return;
            }
            else hasEscapeTarget = false;
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

    if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) >= rangeEscape)
    {
        hasEscapeTarget = false;
        agentAI->ResetSpeed();
    }
}