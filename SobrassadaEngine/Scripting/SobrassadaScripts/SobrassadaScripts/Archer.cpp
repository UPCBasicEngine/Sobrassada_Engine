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

void Archer::HandleState(float deltaTime)
{
    // if (!animComponent) return;

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

void Archer::Attack(float deltaTime)
{
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
        // Enable hitbox when animation hits
        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            hasShot          = true;
            float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
            direction.Normalize();
            arrow->Shoot(parent->GetPosition(), direction);
        }

        // Reset attack state
        if (attackTimer >= attackDuration)
        {
            GLOG("SINGLE ATTACK FINISHED");
            hasShot       = false;
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->SetLookForward(true);

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
    if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) < rangeEscape)
        currentState = ArcherStates::ESCAPE;
    else if (distance <= rangeAIAttack) currentState = ArcherStates::BASIC_ATTACK;
    else if (distance <= rangeAIChase) currentState = ArcherStates::CHASE;
    else if (distance > maxDetectionRange) currentState = ArcherStates::SEARCH;
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