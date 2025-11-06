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
#include "Standalone/MeshComponent.h"
#include "AttackVfxSpritesheet.h"
#include "ShaderScriptComponent.h"
#include "Wwise_IDs.h"
#include <cmath>
bool Archer::triggered = false;
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
    

     const std::vector<GameObject*>* allCoverPoints = scene->GetTaggedGameObjects(HashString("CoverPoint"));

    if (allCoverPoints && !allCoverPoints->empty())
    {
        //GLOG("INIT: Found %d cover points in scene", allCoverPoints->size());

        availableCoverPoints.clear(); 

        for (GameObject* coverPoint : *allCoverPoints)
        {
            if (coverPoint && coverPoint->IsEnabled())
            {
                availableCoverPoints.push_back(coverPoint);
                //GLOG("INIT: Added cover point: %s", coverPoint->GetName().c_str());
            }
        }

        //GLOG("INIT: Total available cover points: %d", availableCoverPoints.size());
    }
    else
    {
        //GLOG("[WARNING] INIT: No cover points found in scene!");
    }

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
           
          
        }
    }

    glowVfxObject = GetGlowEffect();
   if (glowVfxObject)
    {
       
        //GLOG("VFX: Found VFX Glow object!");
        //GLOG("VFX: Successfully found Glow VFX object as sibling");
        ParticleSystemComponent* particleSystem = glowVfxObject->GetComponent<ParticleSystemComponent*>();
        if (!particleSystem) GLOG("[WARNING] VFX object has no ParticleSystemComponent");
        //GLOG("VFX: ParticleSystemComponent found");
        glowVfxObject->SetEnabled(false);
        glowVfxIsActive = false;
        glowTimer       = 0.0f;
    }

    hitVfxObject = GetHitEffect();
    if (hitVfxObject)
    {

        //GLOG("VFX: Found VFX Hit object!");
        //GLOG("VFX: Successfully found Hit VFX object as sibling");
        ParticleSystemComponent* particleSystem = hitVfxObject->GetComponent<ParticleSystemComponent*>();
        if (!particleSystem) GLOG("[WARNING] VFX object has no ParticleSystemComponent");
        //GLOG("VFX: ParticleSystemComponent found");
        hitVfxObject->SetEnabled(false);
        hitVfxIsActive = false;
        hitVfxTimer       = 0.0f;
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
    if (isDead || currentState == ArcherStates::DEATH)
    {
        if (agentAI)
        {
            agentAI->PauseMovement();
            agentAI->SetSpeed(0.0f, 0.0f);
        }

        if (animComponent) animComponent->UseTrigger("die");

        deathTimer += deltaTime;
        if (deathTimer >= DEATH_DURATION)
        {
            parent->SetEnabledRecursive(false);
            //GLOG("Archer DISAPPEARED");
        }

        Character::UpdateTimers(deltaTime);
        return; 
    }
    if (currentState != ArcherStates::DEATH)
    {
        if (playerScript && (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN))
        {
            if (animComponent) animComponent->UseTrigger("idle");
            //GLOG("UPDATE: Player dead, forcing PATROL from state: %s", GetLogicStateName().c_str());

           
            agentAI->SetSpeed(0.0f, 0.0f);
            agentAI->SetLookForward(true);

          
            isAttacking      = false;
            isAiming         = false;
            hasShot          = false;
            isKnockback      = false;
            hasEscapeTarget  = false;
            hasDangerTarget  = false;
            seekingCover     = false;
            isInCover        = false;
            currentCover     = nullptr;
            dangerStuckTimer = 0.0f;
            dangerTimer      = 0.0f;

           
            currentState     = ArcherStates::PATROL;

           
            return;
        }
    }
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

    if  (currentState != ArcherStates::DANGER && currentState != ArcherStates::DEATH &&
            currentState != ArcherStates::ESCAPE && currentState != ArcherStates::AIM &&
            currentState != ArcherStates::PREAIM && currentState != ArcherStates::BASIC_ATTACK)
    {
        float distToPlayer = GetDistanceFromPlayer();

        if (distToPlayer <= rangeEscape)
        {
            //GLOG("EMERGENCY ESCAPE - Player too close at %.2f!", distToPlayer);

            if (isAttacking)
            {
                isAttacking = false;
                attackTimer = 0.0f;
                hasShot     = false;
            }

            if (isAiming)
            {
                isAiming = false;
                aimTimer = 0.0f;
            }

            currentState    = ArcherStates::ESCAPE;
            hasEscapeTarget = false; 
            agentAI->ResetSpeed();
            GLOG("Forced immediate escape from state");
        }
    }

    Character::Update(deltaTime);
    repositionTimer += deltaTime;
    breathDuration  += deltaTime;

    if (currentState == ArcherStates::CHASE && repositionTimer >= 2.0f)
    {
        std::vector<float3> nearbyArchers = GetNearbyArcherPositions();
        if (nearbyArchers.size() > 0)
        {
            targetSpreadPosition = CalculateSpreadPosition();
            hasSpreadPosition    = true;
        }
        else
        {
            hasSpreadPosition = false;
        }
        repositionTimer = 0.0f;
    }

    if (breathDuration >= breathTime) shouldAttack = true;

    if (hitVfxIsActive && hitVfxObject)
    {
        hitVfxTimer += deltaTime;
        /*GLOG(
            "VFX Hit Update: Timer %.3f / %.3f, Enabled: %s", hitVfxTimer, hitVfxDuration,
            hitVfxObject->IsEnabled() ? "YES" : "NO"
        );*/

        if (hitVfxTimer >= hitVfxDuration)
        {
            //GLOG("VFX Hit: Disabling after %.3f seconds", hitVfxTimer);
            hitVfxObject->SetEnabled(false);
            hitVfxIsActive = false;
            hitVfxTimer    = 0.0f;
        }
    }

    if (glowVfxIsActive && glowVfxObject)
    {
        glowTimer += deltaTime;
        /*GLOG(
            "VFX Glow Update: Timer %.3f / %.3f, Enabled: %s", glowTimer, glowVfxDuration,
            glowVfxObject->IsEnabled() ? "YES" : "NO"
        );*/

        if (glowTimer >= glowVfxDuration)
        {
            //GLOG("VFX: Glow Disabling after %.3f seconds", glowTimer);
            glowVfxObject->SetEnabled(false);
            glowVfxIsActive = false;
            glowTimer       = 0.0f;
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
    case ArcherStates::PREAIM:
        return "PREAiming";
        break;
    case ArcherStates::AIM:
        return "Aiming";
        break;
    case ArcherStates::SEARCH:
        return "Search";
        break;
    case ArcherStates::DEATH:
        return "Death";
        break;
    case ArcherStates::DANGER:
        return "DANGER - RETREATING ";
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
        //GLOG("VFX: ERROR - glowVfxObject is NULL!");
        return;
    }

    glowTimer       = 0.0f;
    glowVfxIsActive = true;
    glowVfxObject->SetEnabled(true);

    ParticleSystemComponent* particleSystem = glowVfxObject->GetComponent<ParticleSystemComponent*>();
    if (particleSystem)
    {
        particleSystem->SpawnAllInstances();
        //GLOG("VFX: Glow particles spawned at archer position");
    }
    else
    {
        GLOG("VFX: WARNING - No ParticleSystemComponent found on %s", glowVfxObject->GetName().c_str());
    }
}

void Archer::ActivateHitVFX()
{
 
    if (hitVfxObject)
    {
        //GLOG("VFX: Activating hit effect - Object found: %s", hitVfxObject->GetName().c_str());

        //GLOG("VFX: Activating hit effect - Object found: %s", hitVfxObject->GetName().c_str());
        hitVfxTimer    = 0.0f;
        hitVfxIsActive = true;

       
        hitVfxObject->SetEnabled(true);
        //GLOG("VFX: Object enabled");

        auto meshComp = hitVfxObject->GetComponent<MeshComponent*>(); 
        if (meshComp != nullptr)
        {
            meshComp->SetEnabled(false);
            //GLOG("VFX: MeshComponent disabled successfully");
        }
        else
        {
            //GLOG("VFX: WARNING - No MeshComponent found on %s", hitVfxObject->GetName().c_str());
        }
        auto shaderScriptComp = hitVfxObject->GetComponent<ShaderScriptComponent*>();
        if (shaderScriptComp != nullptr)
        {
            //GLOG("VFX: ShaderScriptComponent found");

            auto attackVfxScript = shaderScriptComp->GetScriptByType<AttackVfxSpritesheet>();
            if (attackVfxScript)
            {
                attackVfxScript->Reset();
                //GLOG("VFX: AttackVfxSpritesheet Reset() called successfully");
            }
            else
            {
                GLOG("VFX: ERROR - AttackVfxSpritesheet script not found!");
            }
        }
        else
        {
            GLOG("VFX: ERROR - No ShaderScriptComponent found on %s", hitVfxObject->GetName().c_str());
        }
        /*ParticleSystemComponent* particleSystem = hitVfxObject->GetComponent<ParticleSystemComponent*>();
        if (particleSystem)
        {
            if (currentHealth >= 2)
                if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_HURT);

            if (currentHealth <= 0)
                if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_DEATH);
                
          
            particleSystem->SpawnAllInstances();
            GLOG("VFX: Hit particles spawned");
        }
        else
        {
            GLOG("VFX: WARNING - No ParticleSystemComponent found on %s", hitVfxObject->GetName().c_str());
        }*/
    }
    else
    {
        GLOG("VFX: ERROR - archerVfxObject is NULL!");
    }
}



bool Archer::CanShootSafely()
{
    if (!character || attackCdTimer > 0.0f) return false;

    float distanceToPlayer = GetDistanceFromPlayer();
    hasLineOfSight         = CheckLineOfSight();

    return (distanceToPlayer >= safeShootingDistance || (currentCover != nullptr)) && hasLineOfSight;
}



float3 Archer::CalculateSpreadPosition()
{
    if (!character)
    {
        //GLOG("CalculateSpreadPosition: No character found");
        return parent->GetPosition();
    }

    float3 playerPos                          = character->GetLastPosition();
    float3 archerPos                          = parent->GetPosition();

    std::vector<float3> nearbyArcherPositions = GetNearbyArcherPositions();
    //GLOG("CalculateSpreadPosition: Found %d nearby archers", nearbyArcherPositions.size());

    if (nearbyArcherPositions.empty())
    {
        //GLOG("CalculateSpreadPosition: No nearby archers, going direct to player");
        return playerPos;
    }

    float spreadRadius = 4.0f;
    float angleStep    = 45.0f * (3.14159f / 180.0f);

    for (int i = 0; i < 12; i++)
    {
        float angle         = angleStep * i;
        float3 offset       = float3(std::cos(angle) * spreadRadius, 0.0f, std::sin(angle) * spreadRadius);
        float3 candidatePos = playerPos + offset;

        /*GLOG(
            "CalculateSpreadPosition: Testing angle %d (%.1f deg), position (%.2f, %.2f, %.2f)", i,
            angle * 180.0f / 3.14159f, candidatePos.x, candidatePos.y, candidatePos.z
        );*/

        bool positionFree = true;
        for (const float3& otherPos : nearbyArcherPositions)
        {
            float distToOther = candidatePos.Distance(otherPos);
            if (distToOther < 3.0f)
            {
                //GLOG("CalculateSpreadPosition: Position blocked by archer at distance %.2f", distToOther);
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
                    /*GLOG(
                        "CalculateSpreadPosition: SUCCESS! Using spread position (%.2f, %.2f, %.2f)", navPosition.x,
                        navPosition.y, navPosition.z
                    );*/
                    return navPosition;
                }
                else
                {
                    GLOG("CalculateSpreadPosition: Navmesh validation failed for position %d", i);
                }
            }
        }
    }

    //GLOG("CalculateSpreadPosition: No spread position found, trying closer positions");

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
                /*GLOG(
                    "CalculateSpreadPosition: Using closer spread position (%.2f, %.2f, %.2f)", navPosition.x,
                    navPosition.y, navPosition.z
                );*/
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
        GLOG("Starting highlight sequence");
        currentState             = ArcherStates::HIGHLIGHTING;
        currentHighlightingState = ArcherHighlightingStates::IDLE;
        highlightTimer           = 0.0f;
        if (animComponent) animComponent->UseTrigger("idle");
    }
  
}

void Archer::UpdateHighlightState(float deltaTime)
{
    highlightTimer += deltaTime;

    switch (currentHighlightingState)
    {
    case ArcherHighlightingStates::IDLE:
        if (highlightTimer >= 1.0f)
        {
            currentHighlightingState = ArcherHighlightingStates::AIM;
            highlightTimer           = 0.0f;
            if (animComponent) animComponent->UseTrigger("aim"); 
        }
        break;

    case ArcherHighlightingStates::AIM:
        if (highlightTimer >= 3.0f)
        {
            currentHighlightingState = ArcherHighlightingStates::BASIC_ATTACK;
            highlightTimer           = 0.0f;
            if (animComponent) animComponent->UseTrigger("attack");
        }
        break;

    case ArcherHighlightingStates::BASIC_ATTACK:
        if (highlightTimer >= 1.0f)
        {
            currentHighlightingState = ArcherHighlightingStates::COOLDOWN;
            highlightTimer           = 0.0f;
        }
        break;

    case ArcherHighlightingStates::COOLDOWN:
        if (highlightTimer >= highlightDuration)
        {
            currentHighlightingState = ArcherHighlightingStates::DONE;
            highlightTimer           = 0.0f;
        }
        break;

    case ArcherHighlightingStates::DONE:
        if (animComponent) animComponent->UseTrigger("idle");
        currentHighlightingState = ArcherHighlightingStates::IDLE;
        currentState             = ArcherStates::PATROL;
        highlightTimer           = 0.0f;
        break;
    }
    
}

void Archer::OnDeath()
{
    isDead          = true;
    currentState    = ArcherStates::DEATH;

    hasEscapeTarget = false;
    hasDangerTarget = false;
    isAiming        = false;
    isAttacking     = false;
    hasShot         = false;
    isKnockback     = false;

    if (hitVfxObject)
    {
        hitVfxObject->SetEnabled(false);
        hitVfxIsActive = false;
        hitVfxTimer    = 0.0f;
    }

    if (glowVfxObject)
    {
        glowVfxObject->SetEnabled(false);
        glowVfxIsActive = false;
        glowTimer       = 0.0f;
    }

    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_ARCHER_DEATH);

    if (agentAI)
    {
        agentAI->PauseMovement();
        agentAI->SetSpeed(0.0f, 0.0f);
    }

    if (animComponent)
    {
        animComponent->UseTrigger("die");
    }

    deathTimer = 0.0f;
}

void Archer::OnDamageTaken(int amount)
{
    if (isDead || currentState == ArcherStates::DEATH) return;
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

   ActivateHitVFX();

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

        //GLOG("OVERSHOOTING STARTED - Pool: %d, Target: %d", arrowPool.size(), numberOfShoots);
    }
    else
    {
      
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!hasStartedShooting && attackTimer >= attackHitboxDelay)
        {
            hasStartedShooting = true;
            currentShot        = 0;
            shotTimer          = 0.0f;
            //GLOG("OVERSHOOTING - MACHINE GUN SEQUENCE STARTED!");
        }

        if (hasStartedShooting && currentShot < numberOfShoots)
        {
            shotTimer += deltaTime;

            if (shotTimer >= shotDelay)
            {
                if (arrowPool.empty())
                {
                    //GLOG("[ERROR] Arrow pool is empty!");
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

                if (arrowGameObject)
                {
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


void Archer::HandleState(float deltaTime)
{
    switch (currentState)
    {
    case ArcherStates::SEARCH:
        SearchForPlayer();
        break;
    case ArcherStates::PATROL:
        PatrolAI();
        if (animComponent) animComponent->UseTrigger("idle");
        break;
    case ArcherStates::CHASE:
        ChaseAI();
        break;
    case ArcherStates::PREAIM:
        PreAim(deltaTime);
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
    case ArcherStates::DANGER:
        DangerRetreat(deltaTime);
        break;
    case ArcherStates::HIGHLIGHTING:
        //GLOG("FRAME: HIGHLIGHTING update called");
        UpdateHighlightState(deltaTime);
        if (currentHighlightingState == ArcherHighlightingStates::DONE)
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
        //GLOG("No state provided to Archer");
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
    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
        return;
    }
   
    if (!playerScript->IsDead() && playerScript->GetState() != CharacterStates::RESPAWN)
    {
        float distance = GetDistanceFromPlayer();

        if (currentState == ArcherStates::PATROL)
        {
            if (!isStatic)
            {

                if (distance <= rangeAIChase)
                {
                    //GLOG("PATROL -> CHASE: Player detected at distance %.2f", distance);
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
    const float3 myPos   = parent->GetGlobalTransform().TranslatePart();
    const float3 origin  = character ? character->GetLastPosition() : float3::zero;

    knockbackDirection   = myPos - origin;
    knockbackDirection.y = 0.0f;
    if (knockbackDirection.LengthSq() < 0.001f) knockbackDirection = float3::unitZ;
    knockbackDirection.Normalize();
}

bool Archer::IsNavmeshPathClear(float3 from, float3 to)
{
    if (!agentAI) return false;

    float hitT   = 0.0f;
    bool success = agentAI->RaycastNavmesh(from, to, hitT);

    if (!success) return false;

    return (hitT >= 0.95f);
}

void Archer::ChaseAI()
{
  if (animComponent) animComponent->UseTrigger("run");

    if (character != nullptr)
    {
        float distance = GetDistanceFromPlayer();

        agentAI->ResetSpeed();
        agentAI->SetSpeed(5.0, 10.0f);
        agentAI->SetLookForward(true);


        const float AIM_THRESHOLD = rangeAIAttack - 0.5f;
        if (distance <= AIM_THRESHOLD && attackCdTimer <= 0.0f)
        {
            currentState = ArcherStates::AIM;
            return;
        }

        bool pathSet = false;

        if (hasSpreadPosition)
        {
            pathSet = agentAI->SetPathNavigation(targetSpreadPosition);
        }

        if (!pathSet)
        {
            agentAI->SetPathNavigation(character->GetLastPosition());
        }
    }
}



void Archer::DangerRetreat(float deltaTime)
{
    if (!agentAI || !character) return;
    float3 archerPos   = parent->GetGlobalTransform().TranslatePart();
    float3 playerPos   = character->GetLastPosition();
    float distToPlayer = archerPos.Distance(playerPos);

    if (animComponent) animComponent->UseTrigger("run");
    agentAI->SetLookForward(true);
   
    if (distToPlayer >= 10.0f)
    {
        hasDangerTarget  = false;
        dangerTimer      = 0.0f;
        dangerStuckTimer = 0.0f;
        agentAI->ResetSpeed();
        ChangeState();
        return;
    }

    
    if (playerScript && playerScript->GetState() != CharacterStates::ULTIMATE)
    {
        dangerTimer += deltaTime;
        if (dangerTimer >= dangerDuration)
        {
            hasDangerTarget  = false;
            dangerTimer      = 0.0f;
            dangerStuckTimer = 0.0f;
            agentAI->ResetSpeed();
            ChangeState();
            return;
        }
    }

   
    if (hasDangerTarget)
    {
        float movement = archerPos.Distance(lastDangerPosition);

        if (movement < 0.1f * deltaTime) 
        {
            dangerStuckTimer += deltaTime;

            if (dangerStuckTimer >= 0.5f) 
            {
                //GLOG("DANGER: Stuck! Finding new escape point");
                hasDangerTarget  = false;
                dangerStuckTimer = 0.0f;

                
                if (distToPlayer >= 6.0f)
                {
                    //GLOG("DANGER: Stuck but safe distance (%.2f), staying here", distToPlayer);
                    agentAI->SetSpeed(0.0f, 0.0f);
                    return;
                }
            }
        }
        else
        {
            dangerStuckTimer = 0.0f; 
        }
    }

    lastDangerPosition = archerPos;

   
    if (!hasDangerTarget)
    {
        float3 escapeDir        = (archerPos - playerPos).Normalized();
        escapeDir.y             = 0.0f;

        float escapeDistance    = 4.0f;
        const float angleStep   = 30.0f * (3.14159f / 180.0f);
        const float3 searchArea = {3.0f, 3.0f, 3.0f};

        bool found              = false;

       
        for (int i = 0; i < 12; i++)
        {
            float angle = angleStep * i;
            float3 dir  = float3(
                escapeDir.x * std::cos(angle) - escapeDir.z * std::sin(angle), 0.0f,
                escapeDir.x * std::sin(angle) + escapeDir.z * std::cos(angle)
            );

            float3 candidatePos = archerPos + dir * escapeDistance;

            bool posOverPoly    = false;
            float3 navPos       = float3::zero;
            agentAI->GetClosestPointInNavmesh(candidatePos, searchArea, posOverPoly, navPos);

           
            if (posOverPoly && candidatePos.Distance(navPos) <= 2.0f &&
                navPos.Distance(playerPos) > distToPlayer + 1.0f &&
                IsNavmeshPathClear(archerPos, navPos)) 
            {
                dangerEscapeTarget = navPos;
                hasDangerTarget    = true;
                found              = true;
                //GLOG("DANGER: Found clear escape at angle %d", i * 30);
                break;
            }
        }

        if (!found)
        {
            
            if (distToPlayer >= 6.0f)
            {
                //GLOG("DANGER: No route but safe (%.2f), holding position", distToPlayer);
                agentAI->SetSpeed(0.0f, 0.0f);
                return;
            }
           
            float3 perpDir     = float3(-escapeDir.z, 0.0f, escapeDir.x);
            dangerEscapeTarget = archerPos + perpDir * 3.0f;
            hasDangerTarget    = true;
        }

        agentAI->SetSpeed(7.0f, 10.0f);
    }

   
    if (hasDangerTarget)
    {
        agentAI->SetPathNavigation(dangerEscapeTarget);

        float distToTarget = archerPos.Distance(dangerEscapeTarget);

       
        if (distToTarget <= 1.5f || distToPlayer >= 9.0f)
        {
            GLOG("DANGER: Escape complete");
            hasDangerTarget  = false;
            dangerTimer      = 0.0f;
            dangerStuckTimer = 0.0f;
            agentAI->ResetSpeed();
            ChangeState();
        }
    }
}


void Archer::SearchForPlayer()
{
    float distance = GetDistanceFromPlayer();

  
    if (!isStatic && distance <= rangeAIChase)
    {
        //GLOG("SEARCH -> CHASE: Player detected at close range %.2f", distance);
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

    bool hasLOS = CheckLineOfSight();

   /* if (!hasLOS)
    {
        //GLOG("AIM -> CHASE", hasLOS ? "YES" : "NO");
        isAiming     = false;
        aimTimer     = 0.0f;
        ChangeState();
        return;
    }*/

    if (!isAiming)
    {
        agentAI->SetSpeed(0.0f, 0.0f);

        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("aim");
        isAiming = true;
        aimTimer = 0.0f;
        float3 predictedTarget = CalculatePredictiveTarget();
        if (playerScript) playerScript->ActivateArrowMark(predictedTarget);
        
        ActivateGlowVFX();
        //GLOG("AIM: Starting to aim at player");
    }
    else
    {
        aimTimer += deltaTime;
        if (character)
        {
            float3 predictedTarget = CalculatePredictiveTarget();
            agentAI->LookAtMovement(predictedTarget, deltaTime);
            if (playerScript) playerScript->SetArrowMark(predictedTarget);
        }

        if (aimTimer >= aimDuration)
        {
            //GLOG("AIM: Finished aiming, shooting now");
            isAiming = false;
            aimTimer = 0.0f;

            if (hasMultipleShoots) currentState = ArcherStates::OVERSHOOTING;
            else currentState = ArcherStates::BASIC_ATTACK;
        }
    }
}

void Archer::PreAim(float deltaTime)
{
    if (!isPreAiming)
    {
        agentAI->SetSpeed(0.0f, 0.0f);

        isPreAiming = true;
        preAimTimer = 0.0f;

        //if (animComponent) animComponent->UseTrigger("aim");

        if (agentAI && character)
        {
            agentAI->SetLookForward(false);
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        }
    }
    else
    {
        preAimTimer += deltaTime;

        if (character && agentAI)
        {
            float3 predictedTarget = CalculatePredictiveTarget();
            agentAI->LookAtMovement(predictedTarget, deltaTime);
        }

        if (preAimTimer >= preAimDuration)
        {
            isPreAiming  = false;
            preAimTimer  = 0.0f;
            currentState = ArcherStates::AIM;
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
        attackTimer = 0.0f;
        agentAI->ResumeMovement();
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
        isAiming     = false;
        aimTimer     = 0.0f;
        currentState = ArcherStates::PATROL;
        return;
    }
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
                    //ActivateGlowVFX();
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
    if (isDead || currentState == ArcherStates::DEATH) return;

    if (currentState == ArcherStates::DANGER)
    {
        //GLOG("ChangeState: In DANGER state, not changing");
        return;
    }

    if (playerScript->IsDead() || playerScript->GetState() == CharacterStates::RESPAWN)
    {
        if (currentState != ArcherStates::PATROL)
        {
            //GLOG("Player dead - switching to patrol");
            currentState = ArcherStates::PATROL;
            agentAI->SetLookForward(true);
            seekingCover = false;
            isInCover    = false;
            currentCover = nullptr;
            if (animComponent) animComponent->UseTrigger("idle");
        }
        return;
    }

    const float distance = GetDistanceFromPlayer();
    hasLineOfSight       = CheckLineOfSight();

    if (isStatic)
    {
        if (distance <= maxDetectionRange)
        {
            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f) currentState = ArcherStates::PREAIM;
            else if (distance <= rangeAIAttack && attackCdTimer > 0.0f) currentState = ArcherStates::SEARCH;
            else currentState = ArcherStates::PREAIM;
        }
        else currentState = ArcherStates::SEARCH;
        return;
    }

    bool healthCompromised = (currentHealth <= 2);

    if (distance <= rangeEscape)
    {
        agentAI->SetLookForward(true);
        currentState = ArcherStates::ESCAPE;
        isInCover    = false;
        seekingCover = false;
        currentCover = nullptr;
    }
    else if (distance < rangeAIAttack && hasLineOfSight) currentState = ArcherStates::PREAIM;
    else if (distance >= rangeAIChase)
    {
        currentState = ArcherStates::CHASE;
        agentAI->SetLookForward(true);
    }
   
    else if (distance > maxDetectionRange)
    {
        currentState = ArcherStates::SEARCH;
        agentAI->SetLookForward(true);
    }
   
    else
    {
        agentAI->SetLookForward(true);
        currentState = ArcherStates::PATROL;
    }
   
}

void Archer::Escape(float deltaTime)
{
    if (!agentAI || !character) return;

    float3 archerPos   = parent->GetGlobalTransform().TranslatePart();
    float distToPlayer = character->GetLastPosition().Distance(archerPos);

    if (animComponent) animComponent->UseTrigger("run");
    agentAI->SetLookForward(true);
    
    if (distToPlayer >= rangeEscape + 2.0f)
    {
        hasEscapeTarget  = false;
        dangerStuckTimer = 0.0f;
        agentAI->ResetSpeed();
        ChangeState();
        return;
    }

    if (hasEscapeTarget)
    {
        float movement = archerPos.Distance(lastDangerPosition);

        if (movement < 0.1f * deltaTime)
        {
            dangerStuckTimer += deltaTime;

            if (dangerStuckTimer >= 0.5f)
            {
                //GLOG("ESCAPE: Stuck! Finding new route");
                hasEscapeTarget  = false;
                dangerStuckTimer = 0.0f;

                if (distToPlayer >= rangeEscape + 1.0f)
                {
                    //GLOG("ESCAPE: Stuck but safe distance");
                    agentAI->ResetSpeed();
                    ChangeState();
                    return;
                }
            }
        }
        else dangerStuckTimer = 0.0f;
    }

    lastDangerPosition = archerPos;

  
   if (!hasEscapeTarget)
    {
        float3 playerPos     = character->GetLastPosition();
        float3 escapeDir     = (archerPos - playerPos).Normalized();
        escapeDir.y          = 0.0f;

        float escapeDistance = 4.0f;

        float angles[]       = {0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 90.0f, -90.0f, 120.0f, -120.0f};

        for (float angleDeg : angles)
        {
            float angle    = angleDeg * (3.14159f / 180.0f);
            float3 testDir = float3(
                escapeDir.x * std::cos(angle) - escapeDir.z * std::sin(angle), 0.0f,
                escapeDir.x * std::sin(angle) + escapeDir.z * std::cos(angle)
            );

            float3 targetPos = archerPos + testDir * escapeDistance;
            float hitT       = 0.0f;

            if (agentAI->RaycastNavmesh(archerPos, targetPos, hitT) && hitT >= 0.90f)
            {
             
                bool posOverPoly        = false;
                float3 validatedPos     = float3::zero;
                const float3 searchArea = {2.0f, 2.0f, 2.0f};

                agentAI->GetClosestPointInNavmesh(targetPos, searchArea, posOverPoly, validatedPos);

                if (posOverPoly && targetPos.Distance(validatedPos) <= 1.0f)
                {
                   
                    float3 beyondTarget = targetPos + testDir * 2.0f;
                    float beyondHitT    = 0.0f;
                    bool hasSpaceBeyond = agentAI->RaycastNavmesh(targetPos, beyondTarget, beyondHitT);

                  
                    if ((hasSpaceBeyond && beyondHitT > 0.5f) || targetPos.Distance(playerPos) > distToPlayer + 3.0f)
                    {
                        currentEscapeTarget = validatedPos;
                        hasEscapeTarget     = true;
                        //GLOG("ESCAPE: Good escape point at %.0f degrees", angleDeg);
                        break;
                    }
                    else
                    {
                        //GLOG("ESCAPE: Rejected %.0f degrees - dead end detected", angleDeg);
                    }
                }
                else
                {
                    /*GLOG(
                        "ESCAPE: Rejected %.0f degrees - near navmesh edge (dist=%.2f)", angleDeg,
                        targetPos.Distance(validatedPos)
                    );*/
                }
            }
        }

        if (!hasEscapeTarget)
        {
            //GLOG("ESCAPE: No good escape found, using immediate perpendicular");
            float3 perpDir      = float3(-escapeDir.z, 0.0f, escapeDir.x);
            currentEscapeTarget = archerPos + perpDir * 1.5f;
            hasEscapeTarget     = true;
        }

        agentAI->SetSpeed(6.0f, 10.0f);
    }
   
    if (hasEscapeTarget)
    {
        agentAI->SetPathNavigation(currentEscapeTarget);

        if (archerPos.Distance(currentEscapeTarget) <= 1.5f || distToPlayer >= rangeEscape + 1.0f)
        {
            hasEscapeTarget  = false;
            dangerStuckTimer = 0.0f;
            agentAI->ResetSpeed();
            ChangeState();
        }
    }
}