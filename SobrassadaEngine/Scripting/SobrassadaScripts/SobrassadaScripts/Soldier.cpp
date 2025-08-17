#include "pch.h"

#include "Application.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "ResourceStateMachine.h"
#include "Soldier.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include <random>

Soldier::Soldier(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Soldier)
{
    fields.push_back({"AI Patrol Point", InspectorField::FieldType::Vec3, &patrolPoint, -1000.0f, 1000.0f});
    fields.push_back({"Knockback Time", InspectorField::FieldType::Float, &knockbackTime, 0.0f, 1.0f});
    fields.push_back({"Knockback Force", InspectorField::FieldType::Float, &knockbackForce, 0.0f, 20.0f});
    fields.push_back({"Second Attack Delay", InspectorField::FieldType::Float, &secondAttackDelay, 0.0f, 1.0f});
    fields.push_back({"Chase Speed", InspectorField::FieldType::Float, &chaseSpeed, 0.0f, 10.0f});
}

bool Soldier::Init()
{
    // GLOG("Initiating Soldier");

    currentState = SoldierStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Soldier")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    
    originalAttackDuration    = attackDuration;
    originalAttackHitboxDelay = attackHitboxDelay;

    return true;
}

void Soldier::Update(float deltaTime)
{
    if (currentState == SoldierStates::DEATH && animComponent && animComponent->IsFinished())
    {
        parent->SetEnabled(false);
    }

    if (currentState == SoldierStates::DEATH || agentAI == nullptr) return;

    if (isKnockback)
    {
        knockbackTimer -= deltaTime;
        agentAI->MoveTo(knockbackForce, knockbackDirection);
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

void Soldier::OnPlayerExitLocation()
{
    currentState = SoldierStates::PATROL;
    agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
}

void Soldier::OnPlayerEnterLocation()
{
    currentState = SoldierStates::PATROL;
    agentAI->SetPathNavigation(startPos);
    reachedPatrolPoint = false;
}

void Soldier::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
    isAttacking = false;
    if (animComponent) animComponent->UseTrigger("death");
    agentAI->PauseMovement();
    currentState = SoldierStates::DEATH;
}

void Soldier::OnDamageTaken(int amount)
{
    isAttacking = false;
    attackTimer = 0.0f;
    if (weaponCollider && weaponCollider->GetEnabled())
    {
        weaponCollider->SetEnabled(false);
    }
    isKnockback    = true;
    knockbackTimer = knockbackTime;
    ApplyKnockback();
    //HashString animStateFromPlayer = GetAnimStateNameFromPlayer();
    //std::string animState               = animStateFromPlayer.GetString();
    //GLOG("Soldier %s damaged with state %s", parent->GetName().c_str(), animState.c_str());
    if (animComponent) animComponent->UseTrigger("damaged");
}

void Soldier::PerformAttack()
{
    // TODO: play basicAttack sound
    // TODO: make interaction with hitboxes with the character
    // TODO: activate and disable the box collider located on one on the gameobjects weapon
    // TODO: trails, particles and animation
}

void Soldier::HandleState(float deltaTime)
{
    if (!animComponent) return;

    switch (currentState)
    {
    case SoldierStates::SEARCH:
        SearchForPlayer();
        break;
    case SoldierStates::PATROL:
        agentAI->ResetSpeed();
        PatrolAI(deltaTime);
        // TODO: patrol animation
        animComponent->UseTrigger("patrol");
        break;
    case SoldierStates::CHASE:
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        animComponent->UseTrigger("run");
        agentAI->SetSpeed(chaseSpeed, 8.0);
        // GLOG("Speed set to %f", patrolSpeed);
        ChaseAI();
        break;
    case SoldierStates::BASIC_ATTACK:
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    case SoldierStates::PLAYER_DETECTION:
        animComponent->UseTrigger("detectPlayer");
        if (animComponent->IsFinished())
        {
            currentState = SoldierStates::CHASE;  
        }
        break;
    default:
        GLOG("No state provided to Soldier");
        currentState = SoldierStates::PATROL;
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        if (currentState == SoldierStates::BASIC_ATTACK) animComponent->UseTrigger("idleCombat");
        else animComponent->UseTrigger("idle");
    }
}

void Soldier::PatrolAI(float deltaTime)
{
    const HashString& playerLocation = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    bool playerInLocation            = parent->HasTag(playerLocation);


    if (!playerScript->IsDead())
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Medium && playerInLocation)
        {
            currentState = SoldierStates::PLAYER_DETECTION;
        }
        else if (CheckDistanceWithPlayer() == PlayerDistances::Close && playerInLocation)
            currentState = SoldierStates::BASIC_ATTACK;
    }

    bool valid = false;
    if (reachedPatrolPoint)
    {
        if (CheckDistanceWithPoint(startPos)) reachedPatrolPoint = false;
        else
        {
            valid = agentAI->SetPathNavigation(startPos);
            agentAI->LookAtMovement(startPos, deltaTime);
        }

    }
    else
    {
        if (CheckDistanceWithPoint(patrolPoint)) reachedPatrolPoint = true;
        else
        {
            valid = agentAI->SetPathNavigation(patrolPoint);
            agentAI->LookAtMovement(patrolPoint, deltaTime);
        }
    }

    
}

void Soldier::ChaseAI()
{
    if (character != nullptr)
    {
        agentAI->SetPathNavigation(character->GetLastPosition());
        ChangeState();
    }
    else currentState = SoldierStates::PATROL;
}

void Soldier::SearchForPlayer()
{
    GLOG("Searching for player");
    // Stands still for a few seconds, if player gets close again chases, if not returns to patrol
    if (!isSearching)
    {
        // TODO: Would be nice to be a "search" animation instead of idle
        animComponent->UseTrigger("idle");
        isSearching = true;
        searchTimer = searchDuration;
        agentAI->SetSpeed(0.0f, 10.0f);
    }

    if (GetDistanceFromPlayer() < maxDetectionRange - 0.5f)
    {
        isSearching = false;
        agentAI->ResetSpeed();
        currentState = SoldierStates::PLAYER_DETECTION;
    }
    else if (searchTimer <= 0.0f)
    {
        isSearching  = false;
        agentAI->SetSpeed(chaseSpeed, 8.0);
        GLOG("Speed set to %f", chaseSpeed);
        currentState = SoldierStates::PATROL;
    }
}

void Soldier::Attack(float deltaTime)
{
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        GLOG("ATTACK ENEMY");
        if (animComponent)
        {
            attackHitboxDelay    = originalAttackHitboxDelay;
            currentAttackTrigger   = ManageAttackAnimations();

            if (currentAttackTrigger && strcmp(currentAttackTrigger, "attack") == 0)
            {
                attackHitboxDelay += 0.4f;
                attackDuration = attackHitboxDelay + 2 * attackHitboxDuration + secondAttackDelay + 0.1f;
            }
            else
            {
                attackDuration = originalAttackDuration;
            }
        }
        Character::Attack(deltaTime);
        agentAI->PauseMovement();
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        // Doble attack
        if (currentAttackTrigger && strcmp(currentAttackTrigger, "attack") == 0)
        {
            bool inFirstWindow =
                attackTimer >= attackHitboxDelay && attackTimer <= attackHitboxDelay + attackHitboxDuration;
            float secondDelay   = attackHitboxDelay + attackHitboxDuration + secondAttackDelay;
            bool inSecondWindow = attackTimer >= secondDelay && attackTimer <= secondDelay + attackHitboxDuration;

            if ((inFirstWindow || inSecondWindow) && !weaponCollider->GetEnabled())
            {
                weaponCollider->SetEnabled(true);
            }
            else if (!inFirstWindow && !inSecondWindow && weaponCollider->GetEnabled())
            {
                weaponCollider->SetEnabled(false);
            }
        }
        else // thrust
        {
            if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
                attackTimer <= attackHitboxDelay + attackHitboxDuration)
            {
                weaponCollider->SetEnabled(true);
            }
            else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
            {
                weaponCollider->SetEnabled(false);
            }
        }

        // Reset attack state
        if (attackTimer >= attackDuration)
        {
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResumeMovement();
            ChangeState();
        }
    }
}

void Soldier::ChangeState()
{
    if (playerScript->IsDead())
    {
        currentState = SoldierStates::DEATH;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    if (distance <= rangeAIAttack) currentState = SoldierStates::BASIC_ATTACK;
    else if (distance <= rangeAIChase) currentState = SoldierStates::CHASE;
    else if (distance > maxDetectionRange) currentState = SoldierStates::SEARCH;
}

void Soldier::ApplyKnockback()
{
    const float3 myPos         = parent->GetGlobalTransform().TranslatePart();
    const float3 origin        = character ? character->GetLastPosition() : float3::zero;

    knockbackDirection   = myPos - origin;
    knockbackDirection.y = 0.0f;
    if (knockbackDirection.LengthSq() < 0.001f) knockbackDirection = float3::unitZ;
    knockbackDirection.Normalize();
}

const char* Soldier::ManageAttackAnimations()
{
    const char* attackTrigger = nullptr;
    if (consecutiveAttack >= 2)
    {
        attackTrigger = "thrust";
        consecutiveThrust = 1;
        consecutiveAttack = 0;
    }
    else if (consecutiveThrust >= 2)
    {
        attackTrigger = "attack";
        consecutiveAttack = 1;
        consecutiveThrust = 0;
    }
    else
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 1);
        bool chooseAttack1 = dis(gen) == 0;

        if (chooseAttack1)
        {
            attackTrigger = "attack";
            consecutiveAttack++;
            consecutiveThrust = 0;
        }
        else
        {
            attackTrigger = "thrust";
            consecutiveThrust++;
            consecutiveAttack = 0;
        }
    }

    animComponent->UseTrigger(attackTrigger);

    return attackTrigger;
}