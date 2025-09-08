#include "pch.h"

#include "Application.h"
#include "Archer.h"
#include "ArcherProjectile.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "ParticleSystemComponent.h"
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

    float3 archerPos  = parent->GetPosition();
    float3 playerPos  = character->GetLastPosition();

    archerPos.y      += 1.5f;
    playerPos.y      += 1.0f;

    if (!walls || walls->empty()) return true;

    float distanceToPlayer   = archerPos.Distance(playerPos);
    float3 directionToPlayer = (playerPos - archerPos).Normalized();

    for (GameObject* wall : *walls)
    {
        if (!wall->IsEnabled()) continue;

        float3 wallPos         = wall->GetPosition();
        float3 archerToWall    = wallPos - archerPos;
        float projectionLength = archerToWall.Dot(directionToPlayer);

        if (projectionLength < 0.5f || projectionLength > distanceToPlayer - 0.5f) continue;

        float3 closestPointOnLine = archerPos + directionToPlayer * projectionLength;
        float distanceToLine      = wallPos.Distance(closestPointOnLine);

        if (distanceToLine < 2.0f)
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

        if (abs((distArcherToCover + distCoverToPlayer) - distArcherToPlayer) < 2.0f) protectionScore = 1.0f;
    }

    return distanceScore + protectionScore * 2.0f;
}

float3 Archer::FindShootingPosition()
{
    if (!currentCover || !character) return parent->GetPosition();

    float3 coverPos  = currentCover->GetPosition();
    float3 playerPos = character->GetLastPosition();

    for (int i = 0; i < 8; i++)
    {
        float angle                      = (i / 8.0f) * 2.0f * 3.14159f;
        float3 offset                    = float3(cos(angle), 0, sin(angle)) * coverRadius;
        float3 candidate                 = coverPos + offset;

        float3 candidateToPlayer         = playerPos - candidate;
        float distToPlayer               = candidateToPlayer.Length();

        bool hasLineOfSightFromCandidate = true;

        if (walls)
        {
            for (GameObject* wall : *walls)
            {
                if (!wall->IsEnabled() || wall == currentCover) continue;

                float3 wallPos            = wall->GetPosition();
                float distCandidateToWall = candidate.Distance(wallPos);
                float distWallToPlayer    = wallPos.Distance(playerPos);

                if (abs((distCandidateToWall + distWallToPlayer) - distToPlayer) < 2.0f &&
                    distCandidateToWall < distToPlayer && distWallToPlayer < distToPlayer)
                {
                    hasLineOfSightFromCandidate = false;
                    break;
                }
            }
        }

        if (hasLineOfSightFromCandidate) return candidate;
    }

    return coverPos;
}

float3 Archer::FindClearShootingPosition()
{
    if (!character) return parent->GetPosition();

    float3 archerPos       = parent->GetPosition();
    float3 playerPos       = character->GetLastPosition();

    const int numPositions = 12;
    const float radius     = 3.0f;

    for (int i = 0; i < numPositions; i++)
    {
        float angle         = (i / float(numPositions)) * 2.0f * 3.14159f;
        float3 offset       = float3(cos(angle) * radius, 0, sin(angle) * radius);
        float3 candidatePos = archerPos + offset;

        if (HasLineOfSightFromPosition(candidatePos, playerPos))
        {
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

    float3 directionToPlayer = (playerPos - archerPos).Normalized();
    float3 closerPos         = archerPos + directionToPlayer * 2.0f;

    if (agentAI)
    {
        bool posOverPoly        = false;
        float3 closestPoint     = float3::zero;
        const float3 searchArea = {1.0f, 2.0f, 1.0f};
        agentAI->GetClosestPointInNavmesh(closerPos, searchArea, posOverPoly, closestPoint);
        if (posOverPoly) return closestPoint;
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

        if (distanceToLine < 1.5f) return false;
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
    else agentAI->SetPathNavigation(coverPosition);
}

void Archer::StayInCover(float deltaTime)
{
    if (!currentCover || !character) return;

    float distanceToPlayer = GetDistanceFromPlayer();

    if (distanceToPlayer >= safeShootingDistance && repositionTimer >= repositionDelay)
    {
        shootingPosition = FindShootingPosition();
        currentState     = ArcherStates::POSITIONING_TO_SHOOT;
        repositionTimer  = 0.0f;
        GLOG("Archer leaving cover to shoot");
    }

    else if (distanceToPlayer < safeShootingDistance * 0.8f)
    {
        agentAI->SetSpeed(0.0f, 0.0f);
        if (animComponent) animComponent->UseTrigger("idle");
    }
}

void Archer::PositionToShoot(float deltaTime)
{
    float distanceToShootPos = parent->GetPosition().Distance(shootingPosition);

    if (distanceToShootPos <= 1.0f)
    {
        if (CheckLineOfSight() && CanShootSafely())
        {
            currentState = ArcherStates::AIM;
            isInCover    = false;
        }
        else currentState = ArcherStates::IN_COVER;
    }
    else
    {
        agentAI->SetPathNavigation(shootingPosition);
        if (animComponent) animComponent->UseTrigger("run");
    }

    if (repositionTimer >= 3.0f)
    {
        currentState    = ArcherStates::IN_COVER;
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
    if (hitVfxIsActive && !archerVfxObject->IsEnabled())
    {
        GLOG("Activating arrow VFX - isActive: %s, timer: %f", hitVfxIsActive ? "true" : "false", hitVfxTimer);

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

    if (!playerScript->IsDead() && playerScript->GetState() != CharacterStates::RESPAWN)
    {
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
        if (CheckLineOfSight() && GetDistanceFromPlayer() <= rangeAIAttack)
        {
            currentState = ArcherStates::AIM;
            return;
        }

        agentAI->SetPathNavigation(character->GetLastPosition());
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

    if (!CheckLineOfSight())
    {
        GLOG("ARCHER LOST LINE OF SIGHT - GOING TO CHASE");
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
        GLOG("PLAYER IS DEAD - GOING TO PATROL");
        currentState = ArcherStates::PATROL;
        seekingCover = false;
        isInCover    = false;
        currentCover = nullptr;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    hasLineOfSight       = CheckLineOfSight();

    if (isStatic)
    {
        if (distance <= maxDetectionRange)
        {
            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f && hasLineOfSight) currentState = ArcherStates::AIM;
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
            seekingCover    = true;
            isInCover       = false;
            coverPosition   = currentCover->GetPosition();
            currentState    = ArcherStates::SEEKING_COVER;
            repositionTimer = 0.0f;
            GLOG("Archer seeking cover at wall: %s", currentCover->GetName().c_str());
        }
        else
        {
            if (distance <= rangeAIAttack && hasLineOfSight) currentState = ArcherStates::AIM;
            else if (distance >= rangeAIChase) currentState = ArcherStates::CHASE;
            else if (distance > maxDetectionRange) currentState = ArcherStates::SEARCH;
            else currentState = ArcherStates::PATROL;
        }
    }

    else if (distance <= rangeAIAttack && hasLineOfSight) currentState = ArcherStates::AIM;
    else if (distance >= rangeAIChase) currentState = ArcherStates::CHASE;
    else if (distance > maxDetectionRange) currentState = ArcherStates::SEARCH;
    else currentState = ArcherStates::PATROL;

    if (isStatic)
    {
        if (distance <= maxDetectionRange)
        {
            if (distance <= rangeAIAttack && attackCdTimer <= 0.0f && hasLineOfSight) currentState = ArcherStates::AIM;
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
        if (distance <= rangeEscape)
        {
            currentState = ArcherStates::ESCAPE;
            chaseTimer   = 0.0f;
        }

        else if (distance <= rangeAIAttack && hasLineOfSight)
        {
            currentState = ArcherStates::AIM;
            chaseTimer   = 0.0f;
        }
        else if (distance >= rangeAIChase) currentState = ArcherStates::CHASE;
        else if (distance > maxDetectionRange)
        {
            currentState = ArcherStates::SEARCH;
            chaseTimer   = 0.0f;
        }
        else
        {
            currentState = ArcherStates::PATROL;
            chaseTimer   = 0.0f;
        }
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

    if (animComponent) animComponent->UseTrigger("run");

    if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) >= rangeEscape)
    {
        hasEscapeTarget = false;
        agentAI->ResetSpeed();
        ChangeState();
    }
}