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

Soldier::Soldier(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Soldier)
{
    fields.push_back({"AI Patrol Point", InspectorField::FieldType::Vec3, &patrolPoint, -1000.0f, 1000.0f});
    fields.push_back({"Knockback Time", InspectorField::FieldType::Float, &knockbackTime, 0.0f, 1.0f});
    fields.push_back({"Knockback Force", InspectorField::FieldType::Float, &knockbackForce, 0.0f, 20.0f});
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

    return true;
}

void Soldier::Update(float deltaTime)
{
    if (agentAI == nullptr) return;

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

void Soldier::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
    parent->SetEnabled(false);
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
    if (animComponent) animComponent->UseTrigger("idle");
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
        PatrolAI();
        // TODO: patrol animation
        animComponent->UseTrigger("run");
        break;
    case SoldierStates::CHASE:
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        animComponent->UseTrigger("run");
        ChaseAI();
        break;
    case SoldierStates::BASIC_ATTACK:
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    default:
        GLOG("No state provided to Soldier");
        currentState = SoldierStates::PATROL;
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        animComponent->UseTrigger("idle");
    }
}

void Soldier::PatrolAI()
{
    if (!playerScript->IsDead())
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Medium) currentState = SoldierStates::CHASE;
        else if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentState = SoldierStates::BASIC_ATTACK;
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
        currentState = SoldierStates::CHASE;
    }
    else if (searchTimer <= 0.0f)
    {
        isSearching  = false;
        currentState = SoldierStates::PATROL;
        agentAI->ResetSpeed();
    }
}

void Soldier::Attack(float deltaTime)
{
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        GLOG("ATTACK ENEMY");
        if (animComponent) animComponent->UseTrigger("attack");
        Character::Attack(deltaTime);
        agentAI->PauseMovement();
    }
    else
    {
        if (!isKnockback)
        {
            // Enable hitbox when animation hits
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
            if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
                attackTimer <= attackHitboxDelay + attackHitboxDuration)
            {
                weaponCollider->SetEnabled(true);
            }
            else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
            {
                weaponCollider->SetEnabled(false);
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
}

void Soldier::ChangeState()
{
    if (playerScript->IsDead())
    {
        currentState = SoldierStates::PATROL;
        return;
    }

    const float distance = GetDistanceFromPlayer();
    if (distance <= rangeAIAttack) currentState = SoldierStates::BASIC_ATTACK;
    else if (distance <= rangeAIChase) currentState = SoldierStates::CHASE;
    else if (distance > maxDetectionRange) currentState = SoldierStates::SEARCH;
}

void Soldier::ApplyKnockback()
{
    float3 myPos         = parent->GetGlobalTransform().TranslatePart();
    knockbackDirection   = character->GetFrontDirection();
    knockbackDirection.y = 0.0f;
    if (knockbackDirection.LengthSq() < 0.001f) knockbackDirection = float3::unitZ;
    knockbackDirection.Normalize();
}