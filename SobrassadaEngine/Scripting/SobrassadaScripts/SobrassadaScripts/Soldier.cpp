#include "pch.h"

#include "Application.h"
#include "Component.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "ResourceStateMachine.h"
#include "Soldier.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"

Soldier::Soldier(GameObject* parent) : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, patrolPoint)
{
    type = CharacterType::Soldier;
}

bool Soldier::Init()
{
    //GLOG("Initiating Soldier");

    currentState = SoldierStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Soldier")
    else
    {
        agentAI->RecreateAgent();
        speed = agentAI->GetSpeed();
    }

    return true;
}

void Soldier::Update(float deltaTime)
{
    Character::Update(deltaTime);

    if (agentAI == nullptr) return;

    float gameTime = AppEngine->GetGameTimer()->GetTime() / 1000.0f;

    if (!CanAttack(gameTime) && !agentAI->IsPaused()) agentAI->PauseMovement();
    else if (CanAttack(gameTime) && agentAI->IsPaused()) agentAI->ResumeMovement();

    if (character != nullptr && currentState != SoldierStates::PATROL && !agentAI->IsPaused())
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
}

void Soldier::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
}

void Soldier::OnDamageTaken(int amount)
{
    // TODO: play soldier take damage sound
    // TODO: particles? and animation
}

void Soldier::PerformAttack()
{
    // TODO: play basicAttack sound
    // TODO: make interaction with hitboxes with the character
    // TODO: activate and disable the box collider located on one on the gameobjects weapon
    // TODO: trails, particles and animation
}

void Soldier::HandleState(float gameTime)
{
    // if (!animComponent) return;

    switch (currentState)
    {
    case SoldierStates::PATROL:
        // GLOG("Soldier Patrolling");
        // animComponent->UseTrigger("idle");
        PatrolAI();
        break;
    case SoldierStates::CHASE:
        // GLOG("Soldier Chasing");
        //  animComponent->UseTrigger("Run");
        ChaseAI();
        break;
    case SoldierStates::BASIC_ATTACK:
        // GLOG("Soldier Basic Attack");
        //  animComponent->UseTrigger("attack");
        Attack(gameTime);
        if (CheckDistanceWithPlayer() != CLOSE) currentState = SoldierStates::CHASE;
        break;
    default:
        GLOG("No state provided to Soldier");
        currentState = SoldierStates::PATROL;
        break;
    }
}

void Soldier::PatrolAI()
{
    float deltaTime = AppEngine->GetGameTimer()->GetDeltaTime();

    if (CheckDistanceWithPlayer() == MEDIUM) currentState = SoldierStates::CHASE;
    else if (CheckDistanceWithPlayer() == CLOSE) currentState = SoldierStates::BASIC_ATTACK;

    bool valid = false;
    if (reachedPatrolPoint)
    {
        if (CheckDistanceWithPoint(startPos)) reachedPatrolPoint = false;
        else valid = agentAI->SetPathNavigation(startPos);

        agentAI->LookAtMovement(startPos, deltaTime);
    }
    else
    {
        if (CheckDistanceWithPoint(patrolPoint)) reachedPatrolPoint = true;
        else valid = agentAI->SetPathNavigation(patrolPoint);

        agentAI->LookAtMovement(patrolPoint, deltaTime);
    }
}

void Soldier::ChaseAI()
{
    if (character != nullptr)
    {
        if (CheckDistanceWithPlayer() == CLOSE) currentState = SoldierStates::BASIC_ATTACK;
        else if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = SoldierStates::PATROL;
    }
    else currentState = SoldierStates::PATROL;
}
