#include "pch.h"

#include "Application.h"
#include "Changeling.h"
#include "Component.h"
#include "CuChulainn.h"
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

Changeling::Changeling(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, CharacterType::Archer)
{
    fields.push_back({"AI Patrol Point", InspectorField::FieldType::Vec3, &patrolPoint, -1000.0f, 1000.0f});
}

bool Changeling::Init()
{
    // GLOG("Initiating Soldier");

    currentState = ChangelingStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Archer")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    return true;
}

void Changeling::Update(float deltaTime)
{
    if (agentAI == nullptr) return;
    Character::Update(deltaTime);
}

void Changeling::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
    parent->SetEnabled(false);
}

void Changeling::OnDamageTaken(int amount)
{
    // TODO: play soldier take damage sound
    // TODO: particles? and animation
}

void Changeling::PerformAttack()
{
    // TODO: play basicAttack sound
    // TODO: make interaction with hitboxes with the character
    // TODO: activate and disable the box collider located on one on the gameobjects weapon
    // TODO: trails, particles and animation
}

void Changeling::HandleState(float deltaTime)
{
    //if (!animComponent) return;

    switch (currentState)
    {
    case ChangelingStates::PATROL:
        // GLOG("Soldier Patrolling");
        PatrolAI();
        break;
    case ChangelingStates::CHASE:
        // GLOG("Soldier Chasing");
        ChaseAI();
        break;
    case ChangelingStates::BASIC_ATTACK:
        // GLOG("Soldier Basic Attack");
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    default:
        GLOG("No state provided to Archer");
        currentState = ChangelingStates::PATROL;
        break;
    }

    //if (animComponent && animComponent->IsFinished())
    //{
    //    // GLOG("FINISH ANIM");
    //    animComponent->UseTrigger("idle");
    //}
}

void Changeling::PatrolAI()
{
    //animComponent->UseTrigger("run");

    if (CheckDistanceWithPlayer() == PlayerDistances::Medium) currentState = ChangelingStates::CHASE;
    else if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentState = ChangelingStates::BASIC_ATTACK;

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

void Changeling::ChaseAI()
{
    //animComponent->UseTrigger("run");

    if (character != nullptr)
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Medium) currentState = ChangelingStates::BASIC_ATTACK;
        else if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = ChangelingStates::PATROL;
    }
    else currentState = ChangelingStates::PATROL;
}

void Changeling::Attack(float deltaTime)
{
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        GLOG("ATTACK ENEMY");
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
            hasShot       = true;
            dashDirection = character->GetLastPosition(); //Position of the player
            isDashing         = true;
            dashTimeRemaining = dashDuration;
            agentAI->SetSpeed(dashSpeed, 100000);
            agentAI->SetPathNavigation(dashDirection);
        }

        // Reset attack state
        if (attackTimer >= attackDuration)
        {
            hasShot       = false;
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->SetLookForward(true);

            if (CheckDistanceWithPlayer() != PlayerDistances::Medium) currentState = ChangelingStates::CHASE;
        }
    }
}