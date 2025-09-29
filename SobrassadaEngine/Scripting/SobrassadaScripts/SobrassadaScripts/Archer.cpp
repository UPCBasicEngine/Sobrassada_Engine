#include "pch.h"

#include "Application.h"
#include "Archer.h"
#include "ArcherProjectile.h"
#include "Component.h"
#include "CoverPointTrigger.h"
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
    fields.push_back({"Breath Time", InspectorField::FieldType::Float, &breathTime, 0.0f, 2.0f});
    fields.push_back({"Is Static ", InspectorField::FieldType::Bool, &isStatic});
    fields.emplace_back("Highlight duration", InspectorField::FieldType::Float, &highlightDuration, 0.1f, 10.0f);
}

bool Archer::Init()
{
    scene        = AppEngine->GetSceneModule()->GetScene();
    walls        = scene->GetTaggedGameObjects(HashString("Wall"));
    soldiers     = scene->GetTaggedGameObjects(HashString("Soldier"));

    currentState = ArcherStates::PATROL;
    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr)
    {
      
    }
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(false);
        speed = agentAI->GetSpeed();
    }

    // Setup arrow pool
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
            else if (objName == archerHitVFX)
            {
                archerVfxObject = obj;
                GLOG("VFX: Found VFX object '%s' as sibling!", objName.c_str());
                GLOG("VFX: Successfully found VFX object as sibling");
                ParticleSystemComponent* particleSystem = archerVfxObject->GetComponent<ParticleSystemComponent*>();
                if (!particleSystem) GLOG("[WARNING] VFX object has no ParticleSystemComponent");
                GLOG("VFX: ParticleSystemComponent found");
                archerVfxObject->SetEnabled(false);
                hitVfxIsActive = false;
                hitVfxTimer    = 0.0f;
            }
          
        }
    }

    glowVfxObject = GetGlowEffect();
   if (glowVfxObject)
    {
       
        GLOG("VFX: Found VFX Glow object!");
        GLOG("VFX: Successfully found Glow VFX object as sibling");
        ParticleSystemComponent* particleSystem = glowVfxObject->GetComponent<ParticleSystemComponent*>();
        if (!particleSystem) GLOG("[WARNING] VFX object has no ParticleSystemComponent");
        GLOG("VFX: ParticleSystemComponent found");
        glowVfxObject->SetEnabled(false);
        glowVfxIsActive = false;
        glowTimer       = 0.0f;
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
            arrow = arrowPool[0];
            for (GameObject* arrowObj : allArrows)
            {
                arrowObj->SetEnabledRecursive(false);
            }
        }
        else GLOG("[WARNING] No arrows found for single shoot archer");
    }
   
   

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] Archer: No audio component found");
    return true;
}

void Archer::Update(float deltaTime)
{
    if (agentAI == nullptr) return;

    if (currentState == ArcherStates::DEATH && animComponent && animComponent->IsFinished())
    {
        parent->SetEnabled(false);
    }

    if (currentState == ArcherStates::DEATH)
    {
        Character::UpdateTimers(deltaTime);
        return;
    }

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
    breathDuration  += deltaTime;

    if (breathDuration >= breathTime) shouldAttack = true;

    if (hitVfxIsActive && archerVfxObject)
    {
        hitVfxTimer += deltaTime;
        GLOG(
            "VFX Update: Timer %.3f / %.3f, Enabled: %s", hitVfxTimer, hitVfxDuration,
            archerVfxObject->IsEnabled() ? "YES" : "NO"
        );

        if (hitVfxTimer >= hitVfxDuration)
        {
            GLOG("VFX: Disabling after %.3f seconds", hitVfxTimer);
            archerVfxObject->SetEnabled(false);
            hitVfxIsActive = false;
            hitVfxTimer    = 0.0f; 
        }
    }

    if (glowVfxIsActive && glowVfxObject)
    {
        glowTimer += deltaTime;
        GLOG(
            "VFX Update: Timer %.3f / %.3f, Enabled: %s", glowTimer, glowVfxDuration,
            glowVfxObject->IsEnabled() ? "YES" : "NO"
        );

        if (glowTimer >= glowVfxDuration)
        {
            GLOG("VFX: Disabling after %.3f seconds", hitVfxTimer);
            archerVfxObject->SetEnabled(false);
            hitVfxIsActive = false;
            hitVfxTimer    = 0.0f; 
        }
    }

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life       = "Health: " + std::to_string(currentHealth);
        const std::string animState  = "Anim state: " + stateName.GetString();
        const std::string logicState = "Logic state: " + GetLogicStateName();

        std::vector<std::pair<std::string, float2>> logs {
            {life,       float2(-50.0f,  -140.0f)},
            {animState,  float2(-80.0f,  -160.0f)},
            {logicState, float2(-110.0f, -180.0F)},
        };

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

    if (!walls || walls->empty()) return true;

    std::vector<LineSegment> sightRays;
    sightRays.push_back(LineSegment(archerPos, playerPos));

    float3 right = float3(1.0f, 0.0f, 0.0f);
    float3 up    = float3(0.0f, 1.0f, 0.0f);
    float offset = 0.3f;

    sightRays.push_back(LineSegment(archerPos + right * offset, playerPos + right * offset));
    sightRays.push_back(LineSegment(archerPos - right * offset, playerPos - right * offset));
    sightRays.push_back(LineSegment(archerPos + up * offset, playerPos + up * offset));
    sightRays.push_back(LineSegment(archerPos - up * offset, playerPos - up * offset));

    std::vector<GameObject*> potentialBlockers;
    for (GameObject* wall : *walls)
    {
        if (wall && wall->IsEnabled())
        {
            potentialBlockers.push_back(wall);
        }
    }

    for (const LineSegment& ray : sightRays)
    {
        GameObject* hitObject = RaycastController::GetRayIntersectionObject(ray, potentialBlockers);
        if (hitObject != nullptr)
        {
            return false;
        }
    }

    return true;
}

bool Archer::HasLineOfSightFromPosition(float3 fromPos, float3 toPos)
{
    fromPos.y += 1.5f;
    toPos.y   += 1.0f;

    LineSegment sightRay(fromPos, toPos);

    std::vector<GameObject*> potentialBlockers;
    for (GameObject* wall : *walls)
    {
        if (wall && wall->IsEnabled())
        {
            potentialBlockers.push_back(wall);
        }
    }

    GameObject* hitObject = RaycastController::GetRayIntersectionObject(sightRay, potentialBlockers);

    if (hitObject != nullptr)
    {
        float3 wallPos                       = hitObject->GetPosition();
        float targetToWallDistance           = (toPos - wallPos).Length();

        const float WALL_PROXIMITY_THRESHOLD = 4.0f;
        if (targetToWallDistance <= WALL_PROXIMITY_THRESHOLD)
        {
            return false;
        }

        const float EXTENDED_PROXIMITY_THRESHOLD = 6.0f;
        if (targetToWallDistance <= EXTENDED_PROXIMITY_THRESHOLD)
        {
            float3 shooterToWall   = (wallPos - fromPos).Normalized();
            float3 shooterToTarget = (toPos - fromPos).Normalized();
            float dotProduct       = shooterToWall.Dot(shooterToTarget);

            if (dotProduct > 0.85f)
            {
                return false;
            }
        }

        return false;
    }

    return true;
}

GameObject* Archer::FindBestCoverPoint()
{
    if (availableCoverPoints.empty() || !character)
    {
        GLOG("FindBestCoverPoint: No available points or no character");
        return nullptr;
    }

    bool debugCalled = false;
    if (!debugCalled)
    {
        DebugCoverPoints();
        debugCalled = true;
    }

    float3 archerPos      = parent->GetPosition();
    float3 playerPos      = character->GetLastPosition();
    GameObject* bestPoint = nullptr;
    float bestScore       = -1.0f;

    GLOG("=== EVALUATING %d AVAILABLE COVER POINTS ===", availableCoverPoints.size());

    for (GameObject* point : availableCoverPoints)
    {
        if (!point || !point->IsEnabled())
        {
            GLOG("  SKIP: Point is null or disabled");
            continue;
        }

        ScriptComponent* scriptComp = point->GetComponent<ScriptComponent*>();
        if (!scriptComp)
        {
            GLOG("  SKIP: %s - No script component", point->GetName().c_str());
            continue;
        }

        CoverPointTrigger* trigger = scriptComp->GetScriptByType<CoverPointTrigger>();
        if (!trigger)
        {
            GLOG("  SKIP: %s - No CoverPointTrigger", point->GetName().c_str());
            continue;
        }

        if (trigger->IsCompromised())
        {
            GLOG("  SKIP: %s - COMPROMISED (player is there)", point->GetName().c_str());
            continue;
        }

        float3 groundPos        = trigger->GetGroundPosition();

        bool posOverPoly        = false;
        float3 navPosition      = float3::zero;
        const float3 searchArea = {15.0f, 15.0f, 15.0f};

        if (agentAI)
        {
            agentAI->GetClosestPointInNavmesh(groundPos, searchArea, posOverPoly, navPosition);

            if (!posOverPoly)
            {
                GLOG("  SKIP: %s - Not in navmesh even with large search area", point->GetName().c_str());
                continue;
            }
            else
            {
                GLOG(
                    "  %s - VALID! Nav position: (%.2f, %.2f, %.2f)", point->GetName().c_str(), navPosition.x,
                    navPosition.y, navPosition.z
                );
            }
        }
        else
        {
            GLOG("  SKIP: %s - No AI agent", point->GetName().c_str());
            continue;
        }

        float distToArcher = archerPos.Distance(navPosition);
        float distToPlayer = navPosition.Distance(playerPos);

        float score        = 1.0f / (1.0f + distToArcher * 0.1f);

        GLOG(
            "  %s - Score: %.2f (DistArcher: %.2f, DistPlayer: %.2f)", point->GetName().c_str(), score, distToArcher,
            distToPlayer
        );

        if (score > bestScore)
        {
            bestScore = score;
            bestPoint = point;
        }
    }

    if (bestPoint)
    {
        GLOG("=== SELECTED: %s (Score: %.2f) ===", bestPoint->GetName().c_str(), bestScore);
    }
    else
    {
        GLOG("=== NO SUITABLE COVER POINT FOUND ===");
    }

    return bestPoint;
}

void Archer::SeekCover(float deltaTime)
{
    GLOG("SEEKING_COVER called - currentCover: %s", currentCover ? currentCover->GetName().c_str() : "NULL");

    if (!currentCover)
    {
        GLOG("SEEKING_COVER: Finding cover point...");
        currentCover = FindBestCoverPoint();

        if (currentCover)
        {
            currentCoverPoint           = currentCover;
            ScriptComponent* scriptComp = currentCover->GetComponent<ScriptComponent*>();
            if (scriptComp)
            {
                CoverPointTrigger* trigger = scriptComp->GetScriptByType<CoverPointTrigger>();
                if (trigger)
                {
                    coverPosition = trigger->GetGroundPosition();
                    GLOG(
                        "SEEKING_COVER: Moving to %s at (%.2f, %.2f, %.2f)", currentCover->GetName().c_str(),
                        coverPosition.x, coverPosition.y, coverPosition.z
                    );
                    repositionTimer = 0.0f;
                }
                else
                {
                    GLOG("SEEKING_COVER: Cover point has no trigger - FAILED");
                    currentState = ArcherStates::CHASE;
                    return;
                }
            }
            else
            {
                GLOG("SEEKING_COVER: Cover point has no script - FAILED");
                currentState = ArcherStates::CHASE;
                return;
            }
        }
        else
        {
            GLOG("SEEKING_COVER: NO COVER FOUND - back to CHASE");
            currentState     = ArcherStates::CHASE;
            flankingFailures = 999;
            if (character && agentAI)
            {
                agentAI->SetPathNavigation(character->GetLastPosition());
            }
            return;
        }
    }

    float distanceToCover = parent->GetPosition().Distance(coverPosition);
    GLOG("SEEKING_COVER: Distance to cover: %.2f", distanceToCover);

    if (distanceToCover <= 3.0f)
    {
        GLOG("SEEKING_COVER: Reached cover position");
        isInCover       = true;
        currentState    = ArcherStates::IN_COVER;
        repositionTimer = 0.0f;
        return;
    }

    if (agentAI)
    {
        bool pathSet = agentAI->SetPathNavigation(coverPosition);
        GLOG("SEEKING_COVER: Path set result: %s", pathSet ? "SUCCESS" : "FAILED");
    }

    if (animComponent) animComponent->UseTrigger("run");

    repositionTimer += deltaTime;

    if (repositionTimer >= 2.0f)
    {
        GLOG("SEEKING_COVER: TIMEOUT - back to CHASE");
        currentState      = ArcherStates::CHASE;
        currentCover      = nullptr;
        currentCoverPoint = nullptr;
        repositionTimer   = 0.0f;
        flankingFailures  = 999;

        if (character && agentAI)
        {
            agentAI->SetPathNavigation(character->GetLastPosition());
        }
    }
}

void Archer::StayInCover(float deltaTime)
{
    if (!currentCoverPoint || !character)
    {
        GLOG("IN_COVER: No cover point - back to CHASE");
        currentState = ArcherStates::CHASE;
        return;
    }

    float distanceToPlayer     = GetDistanceFromPlayer();
    bool hasCurrentLineOfSight = CheckLineOfSight();
    bool playerInCover         = IsPlayerInAnyCoverPoint();

    GLOG(
        "IN_COVER: Distance %.2f, LOS: %s, PlayerInCover: %s", distanceToPlayer,
        hasCurrentLineOfSight ? "TRUE" : "FALSE", playerInCover ? "TRUE" : "FALSE"
    );

    ScriptComponent* scriptComp = currentCoverPoint->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CoverPointTrigger* trigger = scriptComp->GetScriptByType<CoverPointTrigger>();
        if (trigger && trigger->IsCompromised())
        {
            GLOG("IN_COVER: Cover compromised - find new position");
            ForceNewCoverPoint();
            return;
        }
    }

    if (hasCurrentLineOfSight && distanceToPlayer <= rangeAIAttack && attackCdTimer <= 0.0f)
    {
        GLOG("IN_COVER -> AIM (has line of sight)");
        currentState = ArcherStates::AIM;
        isInCover    = false;
        return;
    }

    repositionTimer += deltaTime;
    if (!hasCurrentLineOfSight && repositionTimer >= 2.0f)
    {
        shootingPosition            = FindShootingPosition();
        float distanceToShootingPos = parent->GetPosition().Distance(shootingPosition);

        if (distanceToShootingPos > 1.5f)
        {
            GLOG("IN_COVER -> POSITIONING_TO_SHOOT (flanking)");
            currentState    = ArcherStates::POSITIONING_TO_SHOOT;
            repositionTimer = 0.0f;
            return;
        }
    }

    if (repositionTimer >= 8.0f)
    {
        GLOG("IN_COVER: Too long without action - go aggressive (CHASE)");
        currentState    = ArcherStates::CHASE;
        isInCover       = false;
        repositionTimer = 0.0f;

        if (agentAI)
        {
            agentAI->SetPathNavigation(character->GetLastPosition());
        }
    }
}

void Archer::PositionToShoot(float deltaTime)
{
    float distanceToShootPos = parent->GetPosition().Distance(shootingPosition);

    if (distanceToShootPos <= 1.5f)
    {
        if (CheckLineOfSight() && CanShootSafely())
        {
            GLOG("REACHED SHOOTING POSITION - GOING TO AIM");
            currentState = ArcherStates::AIM;
            isInCover    = false;
        }
        else
        {
            GLOG("NO LOS FROM SHOOTING POSITION - BACK TO COVER");
            currentState = ArcherStates::IN_COVER;
        }
    }
    else
    {
        agentAI->SetPathNavigation(shootingPosition);
        if (animComponent) animComponent->UseTrigger("run");
    }

    if (repositionTimer >= 5.0f)
    {
        GLOG("POSITIONING TIMEOUT - BACK TO COVER");
        currentState    = ArcherStates::IN_COVER;
        repositionTimer = 0.0f;
    }
}

float3 Archer::FindShootingPosition()
{
    if (!currentCover || !character) return parent->GetPosition();

    float3 playerPos            = character->GetLastPosition();

    ScriptComponent* scriptComp = currentCover->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CoverPointTrigger* trigger = scriptComp->GetScriptByType<CoverPointTrigger>();
        if (trigger)
        {
            float3 flankingPos = trigger->GetFlankingPosition(playerPos);

            if (agentAI)
            {
                bool posOverPoly        = false;
                float3 navPos           = float3::zero;
                const float3 searchArea = {2.0f, 2.0f, 2.0f};
                agentAI->GetClosestPointInNavmesh(flankingPos, searchArea, posOverPoly, navPos);

                if (posOverPoly && HasLineOfSightFromPosition(navPos, playerPos))
                {
                    return navPos;
                }
            }
        }
    }

    return parent->GetPosition();
}

std::vector<GameObject*>& Archer::GetAvailableCoverPoints()
{
    return availableCoverPoints;
}

std::vector<GameObject*>& Archer::GetOccupiedCoverPoints()
{
    return occupiedCoverPoints;
}

const std::string Archer::GetLogicStateName()
{
    switch (currentState)
    {
    case ArcherStates::PATROL:
        return "Idle/Patrol";
        break;
    case ArcherStates::CHASE:
        return "Chase";
        break;
    case ArcherStates::BASIC_ATTACK:
        return "Attack";
        break;
    case ArcherStates::OVERSHOOTING:
        return "OverShoot";
        break;
    case ArcherStates::AIM:
        return "Aiming";
        break;
    case ArcherStates::SEARCH:
        return "Search";
        break;
    case ArcherStates::SEEKING_COVER:
        return "Seek Cover";
        break;
    case ArcherStates::IN_COVER:
        return "IN COVER";
        break;
    case ArcherStates::DEATH:
        return "Death";
        break;
    case ArcherStates::POSITIONING_TO_SHOOT:
        return "Position to shoot ";
        break;
    case ArcherStates::ESCAPE:
        return "ESCAPE ";
        break;
    default:
        return "MISSING!";
        break;
    }
}

void Archer::ActivateGlowVFX()
{
    if (!glowVfxObject)
    {
        GLOG("VFX: ERROR - glowVfxObject is NULL!");
        return;
    }

    glowTimer       = 0.0f;
    glowVfxIsActive = true;
    glowVfxObject->SetEnabled(true);

    ParticleSystemComponent* particleSystem = glowVfxObject->GetComponent<ParticleSystemComponent*>();
    if (particleSystem)
    {
        particleSystem->SpawnAllInstances();
        GLOG("VFX: Glow particles spawned at archer position");
    }
    else
    {
        GLOG("VFX: WARNING - No ParticleSystemComponent found on %s", glowVfxObject->GetName().c_str());
    }
}

bool Archer::CanShootSafely()
{
    if (!character || attackCdTimer > 0.0f) return false;

    float distanceToPlayer = GetDistanceFromPlayer();
    hasLineOfSight         = CheckLineOfSight();

    return (distanceToPlayer >= safeShootingDistance || (currentCover != nullptr)) && hasLineOfSight;
}

void Archer::ReleaseCoverPoint()
{
    if (currentCoverPoint)
    {
        auto it = std::find(availableCoverPoints.begin(), availableCoverPoints.end(), currentCoverPoint);
        if (it == availableCoverPoints.end())
        {
            availableCoverPoints.push_back(currentCoverPoint);
        }

        auto occupiedIt = std::find(occupiedCoverPoints.begin(), occupiedCoverPoints.end(), currentCoverPoint);
        if (occupiedIt != occupiedCoverPoints.end())
        {
            occupiedCoverPoints.erase(occupiedIt);
        }

        GLOG("Released cover point %s", currentCoverPoint->GetName().c_str());
        currentCoverPoint = nullptr;
    }
}

void Archer::ForceNewCoverPoint()
{
    GLOG("COVER POINT COMPROMISED - Finding new cover point");

    ReleaseCoverPoint();

    if (currentState == ArcherStates::SEEKING_COVER || currentState == ArcherStates::IN_COVER)
    {
        seekingCover    = false;
        isInCover       = false;
        currentCover    = nullptr;
        repositionTimer = 0.0f;
        currentState    = ArcherStates::CHASE;
    }
}

GameObject* Archer::GetCurrentCoverPoint()
{
    return currentCoverPoint;
}

void Archer::DebugCoverPoints()
{
    GLOG("=== DEBUGGING ALL COVER POINTS ===");

    const std::vector<GameObject*>* allCoverPoints = scene->GetTaggedGameObjects(HashString("CoverPoint"));
    if (!allCoverPoints)
    {
        GLOG("No cover points found in scene!");
        return;
    }

    GLOG("Found %d cover points in scene", allCoverPoints->size());

    for (int i = 0; i < allCoverPoints->size(); i++)
    {
        GameObject* point = (*allCoverPoints)[i];
        if (!point)
        {
            GLOG("  [%d] NULL POINTER", i);
            continue;
        }

        GLOG(
            "  [%d] %s - Position: (%.2f, %.2f, %.2f)", i, point->GetName().c_str(), point->GetPosition().x,
            point->GetPosition().y, point->GetPosition().z
        );

        ScriptComponent* scriptComp = point->GetComponent<ScriptComponent*>();
        if (!scriptComp)
        {
            GLOG("       NO SCRIPT COMPONENT!");
            continue;
        }

        CoverPointTrigger* trigger = scriptComp->GetScriptByType<CoverPointTrigger>();
        if (!trigger)
        {
            GLOG("       NO COVER POINT TRIGGER!");
            continue;
        }

        GLOG("       Compromised: %s", trigger->IsCompromised() ? "YES" : "NO");

        float3 groundPos = trigger->GetGroundPosition();
        GLOG("       Ground position: (%.2f, %.2f, %.2f)", groundPos.x, groundPos.y, groundPos.z);

        if (agentAI)
        {
            bool posOverPoly        = false;
            float3 navPosition      = float3::zero;
            const float3 searchArea = {5.0f, 5.0f, 5.0f};
            agentAI->GetClosestPointInNavmesh(groundPos, searchArea, posOverPoly, navPosition);

            GLOG("       Navmesh test: %s", posOverPoly ? "SUCCESS" : "FAILED");
            if (posOverPoly)
            {
                GLOG("       Nav position: (%.2f, %.2f, %.2f)", navPosition.x, navPosition.y, navPosition.z);
            }
        }
    }
}

bool Archer::IsPlayerInAnyCoverPoint()
{
    if (!character) return false;

    const std::vector<GameObject*>* allCoverPoints = scene->GetTaggedGameObjects(HashString("CoverPoint"));
    if (!allCoverPoints) return false;

    for (GameObject* coverPoint : *allCoverPoints)
    {
        if (!coverPoint || !coverPoint->IsEnabled()) continue;

        ScriptComponent* scriptComp = coverPoint->GetComponent<ScriptComponent*>();
        if (scriptComp)
        {
            CoverPointTrigger* trigger = scriptComp->GetScriptByType<CoverPointTrigger>();
            if (trigger && trigger->IsCompromised())
            {
                GLOG("*** PLAYER IS IN COVER: %s (detected by trigger) ***", coverPoint->GetName().c_str());
                return true;
            }
        }
    }

    return false;
}

float3 Archer::CalculateSpreadPosition()
{
    if (!character)
    {
        GLOG("CalculateSpreadPosition: No character found");
        return parent->GetPosition();
    }

    float3 playerPos                          = character->GetLastPosition();
    float3 archerPos                          = parent->GetPosition();

    std::vector<float3> nearbyArcherPositions = GetNearbyArcherPositions();
    GLOG("CalculateSpreadPosition: Found %d nearby archers", nearbyArcherPositions.size());

    if (nearbyArcherPositions.empty())
    {
        GLOG("CalculateSpreadPosition: No nearby archers, going direct to player");
        return playerPos;
    }

    float spreadRadius = 4.0f;
    float angleStep    = 45.0f * (3.14159f / 180.0f);

    for (int i = 0; i < 12; i++)
    {
        float angle         = angleStep * i;
        float3 offset       = float3(std::cos(angle) * spreadRadius, 0.0f, std::sin(angle) * spreadRadius);
        float3 candidatePos = playerPos + offset;

        GLOG(
            "CalculateSpreadPosition: Testing angle %d (%.1f deg), position (%.2f, %.2f, %.2f)", i,
            angle * 180.0f / 3.14159f, candidatePos.x, candidatePos.y, candidatePos.z
        );

        bool positionFree = true;
        for (const float3& otherPos : nearbyArcherPositions)
        {
            float distToOther = candidatePos.Distance(otherPos);
            if (distToOther < 3.0f)
            {
                GLOG("CalculateSpreadPosition: Position blocked by archer at distance %.2f", distToOther);
                positionFree = false;
                break;
            }
        }

        if (positionFree)
        {
            if (agentAI)
            {
                bool posOverPoly        = false;
                float3 navPosition      = float3::zero;
                const float3 searchArea = {5.0f, 5.0f, 5.0f};

                agentAI->GetClosestPointInNavmesh(candidatePos, searchArea, posOverPoly, navPosition);

                if (posOverPoly)
                {
                    GLOG(
                        "CalculateSpreadPosition: SUCCESS! Using spread position (%.2f, %.2f, %.2f)", navPosition.x,
                        navPosition.y, navPosition.z
                    );
                    return navPosition;
                }
                else
                {
                    GLOG("CalculateSpreadPosition: Navmesh validation failed for position %d", i);
                }
            }
        }
    }

    GLOG("CalculateSpreadPosition: No spread position found, trying closer positions");

    float closerRadius = 2.0f;
    for (int i = 0; i < 12; i++)
    {
        float angle         = angleStep * i;
        float3 offset       = float3(std::cos(angle) * closerRadius, 0.0f, std::sin(angle) * closerRadius);
        float3 candidatePos = playerPos + offset;

        if (agentAI)
        {
            bool posOverPoly        = false;
            float3 navPosition      = float3::zero;
            const float3 searchArea = {3.0f, 3.0f, 3.0f};

            agentAI->GetClosestPointInNavmesh(candidatePos, searchArea, posOverPoly, navPosition);

            if (posOverPoly)
            {
                GLOG(
                    "CalculateSpreadPosition: Using closer spread position (%.2f, %.2f, %.2f)", navPosition.x,
                    navPosition.y, navPosition.z
                );
                return navPosition;
            }
        }
    }

    float3 randomOffset = float3(
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f, 0.0f,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f
    );

    float3 fallbackPos = playerPos + randomOffset;
    GLOG(
        "CalculateSpreadPosition: Using random offset fallback (%.2f, %.2f, %.2f)", fallbackPos.x, fallbackPos.y,
        fallbackPos.z
    );

    return fallbackPos;
}

std::vector<float3> Archer::GetNearbyArcherPositions()
{
    std::vector<float3> positions;

    const std::vector<GameObject*>* allArchers = scene->GetTaggedGameObjects(HashString("Archer"));
    if (!allArchers) return positions;

    float3 myPos = parent->GetPosition();

    for (GameObject* archerObj : *allArchers)
    {
        if (!archerObj || archerObj == parent || !archerObj->IsEnabled()) continue;

        float3 otherPos = archerObj->GetPosition();
        float distance  = myPos.Distance(otherPos);

        if (distance <= 10.0f)
        {
            positions.push_back(otherPos);
        }
    }

    return positions;
}

void Archer::OnPlayerExitLocation()
{
    ReleaseCoverPoint();
    currentState = ArcherStates::PATROL;
    agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
    seekingCover       = false;
    isInCover          = false;
    currentCover       = nullptr;
    escapeTimeout      = 0.0f;
}

void Archer::OnPlayerEnterLocation()
{
    currentState = ArcherStates::SEARCH;
}

void Archer::PlayHighlightSequence()
{

    if (currentState == ArcherStates::PATROL)
    {
        currentState             = ArcherStates::HIGHLIGHTING;
        currentHighlightingState = ArcherHighlightingStates::IDLE;
        stateTimer               = 0.0f;
    }
}

void Archer::UpdateHighlightState(float deltaTime)
{
    if (!animComponent)
    {
        currentState = ArcherStates::PATROL;
        return;
    }
    stateTimer -= deltaTime;
    switch (currentHighlightingState)
    {
    case ArcherHighlightingStates::IDLE:
        animComponent->UseTrigger("idle");
        currentHighlightingState = ArcherHighlightingStates::AIM;
        break;

    case ArcherHighlightingStates::AIM:
        if (animComponent->IsFinished())
        {
            animComponent->UseTrigger("aim");
            currentHighlightingState = ArcherHighlightingStates::BASIC_ATTACK;
        }
        break;

    case ArcherHighlightingStates::BASIC_ATTACK:

        if (animComponent->IsFinished())
        {
            animComponent->UseTrigger("attack");
            stateTimer               = highlightDuration;
            currentHighlightingState = ArcherHighlightingStates::COOLDOWN;
        }
        break;

    case ArcherHighlightingStates::COOLDOWN:
        if (stateTimer <= 0.0f)
        {
            currentHighlightingState = ArcherHighlightingStates::DONE;
        }
        break;

    case ArcherHighlightingStates::DONE:
        animComponent->UseTrigger("idle");
        currentHighlightingState = ArcherHighlightingStates::IDLE;
        currentState             = ArcherStates::AIM;
        break;
    }
}

void Archer::OnDeath()
{
    ReleaseCoverPoint();
    isAttacking  = false;
    currentState = ArcherStates::DEATH;
    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_DEATH);
    if (animComponent)
    {

        GLOG("TRIGGERING die ANIMATION");
        animComponent->UseTrigger("die");
    }
}

void Archer::OnDamageTaken(int amount)
{

    isAttacking   = false;
    attackTimer   = 0.0f;
    isAiming      = false;
    aimTimer      = 0.0f;
    escapeTimeout = 0.0f;

    if (weaponCollider && weaponCollider->GetEnabled())
    {
        weaponCollider->SetEnabled(false);
    }

    isKnockback    = true;
    knockbackTimer = knockbackTime;
    ApplyKnockback();

    if (archerVfxObject)
    {
        GLOG("VFX: Activating hit effect - Object found: %s", archerVfxObject->GetName().c_str());

        hitVfxTimer    = 0.0f;
        hitVfxIsActive = true;
        archerVfxObject->SetEnabled(true);

        ParticleSystemComponent* particleSystem = archerVfxObject->GetComponent<ParticleSystemComponent*>();
        if (particleSystem)
        {
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_HURT);
            particleSystem->SpawnAllInstances();
            GLOG("VFX: Hit particles spawned");
        }
        else
        {
            GLOG("VFX: WARNING - No ParticleSystemComponent found on %s", archerVfxObject->GetName().c_str());
        }
    }
    else
    {
        GLOG("VFX: ERROR - archerVfxObject is NULL!");
    }

    if (animComponent)
    {
        animComponent->UseTrigger("damageSmall");
    }
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
        if (shouldAttack)
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
                    float3 baseDirection =
                        (predictedTarget - parent->GetGlobalTransform().TranslatePart()).Normalized();

                    float spreadAngle              = 10.0f * (3.14159f / 180.0f);
                    float randomAngle              = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spreadAngle;
                    float3 shootDirection          = baseDirection;
                    float cosA                     = std::cos(randomAngle);
                    float sinA                     = std::sin(randomAngle);
                    float x                        = shootDirection.x * cosA - shootDirection.z * sinA;
                    float z                        = shootDirection.x * sinA + shootDirection.z * cosA;
                    shootDirection.x               = x;
                    shootDirection.z               = z;

                    float3 arrowPos                = float3(parent->GetPosition().x, 1.3f, parent->GetPosition().z);

                    ArcherProjectile* currentArrow = arrowPool[currentArrowIndex];
                    GameObject* arrowGameObject    = currentArrow->GetParent();

                    if (arrowGameObject)
                    {
                        ActivateGlowVFX();
                        arrowGameObject->SetEnabled(true);
                        arrowGameObject->SetEnabledRecursive(true);

                        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_TRI_ATTACK);
                        if (animComponent) animComponent->UseTrigger("multi");
                        currentArrow->Shoot(arrowPos, shootDirection);
                    }

                    currentShot++;
                    currentArrowIndex = (currentArrowIndex + 1) % arrowPool.size();
                    shotTimer         = 0.0f;
                }
            }

            bool allShotsFired = (currentShot >= numberOfShoots);
            bool timeExpired   = (attackTimer >= attackDuration);

            if (allShotsFired || timeExpired)
            {
                hasShot            = false;
                isAttacking        = false;
                hasStartedShooting = false;
                currentShot        = 0;
                shotTimer          = 0.0f;
                attackCdTimer      = attackCooldown;
                agentAI->ResetSpeed();
                agentAI->SetLookForward(true);
                isAiming       = false;
                aimTimer       = 0.0f;
                breathDuration = 0.0f;
                shouldAttack   = false;

                ChangeState();
                return;
            }
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
    case ArcherStates::HIGHLIGHTING:
        UpdateHighlightState(deltaTime);
        if (currentHighlightingState == ArcherHighlightingStates::DONE || triggeredSequence)
        {
            float distToPlayer = GetDistanceFromPlayer();
            currentState       = (distToPlayer <= rangeAIAttack) ? ArcherStates::CHASE : ArcherStates::SEARCH;
        }
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

    if (!playerScript->IsDead() && playerScript->GetState() != CharacterStates::RESPAWN)
    {
        float distance = GetDistanceFromPlayer();

        if (isStatic)
        {
            if (distance <= maxDetectionRange && currentState == ArcherStates::PATROL)
            {
                if (currentHighlightingState == ArcherHighlightingStates::IDLE)
                {
                    GLOG("STATIC PATROL -> HIGHLIGHTING: Player detected at distance %.2f", distance);
                    PlayHighlightSequence();
                    return;
                }
            }
        }
        else
        {
            if (distance <= maxDetectionRange && distance > rangeAIChase && currentState == ArcherStates::PATROL)
            {
                if (currentHighlightingState == ArcherHighlightingStates::IDLE)
                {
                    GLOG("MOBILE PATROL -> HIGHLIGHTING: Player detected at distance %.2f", distance);
                    PlayHighlightSequence();
                    return;
                }
            }
        }

        if (currentState == ArcherStates::PATROL)
        {
            if (!isStatic)
            {

                if (distance <= rangeAIChase)
                {
                    GLOG("PATROL -> CHASE: Player detected at distance %.2f", distance);
                    currentState = ArcherStates::CHASE;
                    agentAI->ResetSpeed();
                    return;
                }
                else if (distance <= maxDetectionRange)
                {
                    GLOG("PATROL -> SEARCH: Player in detection range %.2f", distance);
                    currentState = ArcherStates::SEARCH;
                    agentAI->ResetSpeed();
                    return;
                }
            }
        }
    }

    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
        return;
    }

    if (isStatic)
    {
        float distance = GetDistanceFromPlayer();
        if (currentState == ArcherStates::PATROL && distance <= rangeAIAttack)
        {
            currentState = ArcherStates::AIM;
        }
        return;
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
        float distance = GetDistanceFromPlayer();

        agentAI->ResetSpeed();
        agentAI->SetSpeed(10.0, 10.0f);
        agentAI->SetLookForward(true);

        if (distance <= rangeEscape)
        {
            GLOG("CHASE -> ESCAPE (too close)");
            currentState = ArcherStates::ESCAPE;
            return;
        }

        if (distance <= rangeAIAttack * 0.8f && attackCdTimer <= 0.0f)
        {
            GLOG("CHASE -> AIM (in close attack range)");
            currentState = ArcherStates::AIM;
            return;
        }
        std::vector<float3> nearbyArchers = GetNearbyArcherPositions();
        bool pathSet                      = false;
        float3 targetPosition             = float3::zero;
        if (nearbyArchers.size() > 0)
        {
            targetPosition = CalculateSpreadPosition();

            if (targetPosition.x != parent->GetPosition().x && targetPosition.y != parent->GetPosition().y &&
                targetPosition.z && parent->GetPosition().z) 
            {
                pathSet = agentAI->SetPathNavigation(targetPosition);
                GLOG(
                    "CHASE: Trying spread position (%.2f, %.2f, %.2f) - Result: %s", targetPosition.x, targetPosition.y,
                    targetPosition.z, pathSet ? "SUCCESS" : "FAILED"
                );
            }
        }

        if (!pathSet)
        {
            targetPosition = character->GetLastPosition();
            pathSet        = agentAI->SetPathNavigation(targetPosition);
            GLOG(
                "CHASE: Fallback to direct player position (%.2f, %.2f, %.2f) - Result: %s", targetPosition.x,
                targetPosition.y, targetPosition.z, pathSet ? "SUCCESS" : "FAILED"
            );
        }

        if (!pathSet && character)
        {
            agentAI->LookAtMovement(character->GetLastPosition(), 0.016f);
            GLOG("CHASE: No valid path found, only rotating towards player");
        }
    }
}


void Archer::SearchForPlayer()
{
    float distance = GetDistanceFromPlayer();

    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        isSearching  = false;
        currentState = ArcherStates::PATROL;
        agentAI->ResetSpeed();
        return;
    }

  
    if (!isStatic && distance <= rangeAIChase)
    {
        GLOG("SEARCH -> CHASE: Player detected at close range %.2f", distance);
        isSearching = false;
        agentAI->ResetSpeed();
        currentState = ArcherStates::CHASE;
        return;
    }

    if (isStatic)
    {
      
        if (animComponent) animComponent->UseTrigger("aim");

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
        else if (agentAI)
        {
            agentAI->SetSpeed(0.0f, 0.0f);
          
            currentState = ArcherStates::PATROL;
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

        if (distance < maxDetectionRange - 0.5f)
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

    float distance = GetDistanceFromPlayer();
    if (!weaponCollider) return;

    bool playerInAnyCover = IsPlayerInAnyCoverPoint();
    bool hasLOS           = CheckLineOfSight();

    if (playerInAnyCover && !hasLOS)
    {
        GLOG("AIM -> CHASE (player in cover: %s, LOS: %s)", playerInAnyCover ? "YES" : "NO", hasLOS ? "YES" : "NO");
        isAiming     = false;
        aimTimer     = 0.0f;
        currentState = ArcherStates::CHASE;
        return;
    }

    if (!isAiming)
    {
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("aim");
        isAiming = true;
        aimTimer = 0.0f;
        GLOG("AIM: Starting to aim at player");
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
            GLOG("AIM: Finished aiming, shooting now");
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
    if (shouldAttack)
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
        }
        else
        {
            float3 predictedTarget = CalculatePredictiveTarget();
            agentAI->LookAtMovement(predictedTarget, deltaTime);

            if (!hasShot && attackTimer >= attackHitboxDelay)
            {
                /* if (!CheckLineOfSight())
                 {
                     hasShot      = false;
                     isAttacking  = false;
                     currentState = ArcherStates::CHASE;
                     return;
                 }*/
                hasShot = true;
                if (!arrow) return;

                float3 predictedTarget = CalculatePredictiveTarget();
                float3 direction       = (predictedTarget - parent->GetGlobalTransform().TranslatePart()).Normalized();
                float3 arrowPos        = float3(parent->GetPosition().x, 1.3f, parent->GetPosition().z);

                GameObject* arrowObj   = arrow->GetParent();
                if (arrowObj)
                {
                    ActivateGlowVFX();
                    arrowObj->SetEnabled(true);
                    arrowObj->SetEnabledRecursive(true);
                    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_ATTACK);
                    arrow->Shoot(arrowPos, direction);
                }
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
                breathDuration = 0.0f;
                shouldAttack   = false;

                ChangeState();
            }
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

    
    bool healthCompromised = (currentHealth <= 2);
    bool playerInCover     = IsPlayerInAnyCoverPoint();

    if (healthCompromised && !isInCover && currentState != ArcherStates::SEEKING_COVER &&
        currentState != ArcherStates::IN_COVER && currentState != ArcherStates::POSITIONING_TO_SHOOT &&
        distance > rangeEscape) 
    {
        GLOG("ChangeState: SEEKING_COVER - health compromised (%d HP)", currentHealth);
        currentState = ArcherStates::SEEKING_COVER;
        seekingCover = true;
        return;
    }

    if (healthCompromised && (currentState == ArcherStates::SEEKING_COVER || currentState == ArcherStates::IN_COVER ||
                              currentState == ArcherStates::POSITIONING_TO_SHOOT))
    {
        if (isInCover)
        {
            if (playerInCover)
            {
             
                currentState = ArcherStates::POSITIONING_TO_SHOOT;
                return;
            }
            else
            {
             
                currentState = ArcherStates::CHASE;
                isInCover    = false;
                return;
            }
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
        float distanceToTarget = (archerPos - currentEscapeTarget).Length();

        if (distanceToTarget < 0.5f)
        {
            hasEscapeTarget = false;
            GLOG("ESCAPE: Reached escape target");
        }
        else
        {
            agentAI->GetClosestPointInNavmesh(currentEscapeTarget, searchArea, posOverPoly, closestPoint);
            if (posOverPoly)
            {
                if (distanceToTarget > 2.0f)
                {

                    agentAI->SetSpeed(20.0f, 5.0f);
                    GLOG("ESCAPE: Running to target - distance %.2f", distanceToTarget);
                }
                else
                {

                    agentAI->SetSpeed(35.0f, 12.0f);
                    if (animComponent) animComponent->UseTrigger("dashStart");
                    GLOG("ESCAPE: DASHING to target - distance %.2f", distanceToTarget);
                }

                agentAI->SetPathNavigation(currentEscapeTarget);
                agentAI->LookAtMovement(currentEscapeTarget, deltaTime);

                float playerDistance = character->GetLastPosition().Distance(archerPos);
                if (playerDistance >= rangeEscape + 1.0f)
                {
                    hasEscapeTarget = false;
                    if (animComponent) animComponent->UseTrigger("dashEnd");
                    agentAI->ResetSpeed();
                    GLOG("ESCAPE: Safe distance reached - %.2f", playerDistance);
                    ChangeState();
                }
                return;
            }
            else
            {
                hasEscapeTarget = false;
                GLOG("ESCAPE: Target not in navmesh, finding new target");
            }
        }
    }

    const float3 playerPos = character->GetLastPosition();
    float3 escapeDir       = archerPos - playerPos;
    escapeDir.y            = 0.0f;
    if (escapeDir.LengthSq() < 0.0001f) escapeDir = float3::unitZ;
    escapeDir.Normalize();

    float escapeDistance  = rangeEscape + 3.0f;
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
            GLOG(
                "ESCAPE: New target found at (%.2f, %.2f, %.2f)", currentEscapeTarget.x, currentEscapeTarget.y,
                currentEscapeTarget.z
            );
            break;
        }
        angleAccum += angleStep;
    }

    if (!found)
    {

        currentEscapeTarget = archerPos + escapeDir * 3.0f;
        hasEscapeTarget     = true;
        GLOG("ESCAPE: Using fallback straight escape");
    }

    float initialDistance = (archerPos - currentEscapeTarget).Length();
    if (initialDistance > 2.0f)
    {
        agentAI->SetSpeed(20.0f, 5.0f);
        GLOG("ESCAPE: Starting run to new target");
    }
    else
    {
        agentAI->SetSpeed(35.0f, 12.0f);
        if (animComponent) animComponent->UseTrigger("dashStart");
        GLOG("ESCAPE: Starting dash to new target");
    }

    agentAI->SetPathNavigation(currentEscapeTarget);
    agentAI->LookAtMovement(currentEscapeTarget, deltaTime);
    agentAI->SetSpeed(25.0f, 8.0f);

    if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) >= rangeEscape)
    {
        hasEscapeTarget = false;
        if (animComponent) animComponent->UseTrigger("dashEnd");
        agentAI->ResetSpeed();
    }
}