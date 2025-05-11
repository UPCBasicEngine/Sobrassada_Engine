#include "pch.h"

#include "Component.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "Globals.h"
#include "ResourceStateMachine.h"
#include "Soldier.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"

Soldier::Soldier(GameObject* parent) : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 1.0f, 2.0f, 10.0f)
{
}

bool Soldier::Init()
{
    //GLOG("Initiating Soldier");

    currentState = SoldierStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (!agentAI)
    {
        GLOG("AIAgent component not found for Soldier");
        return false;
    }

    agentAI->SetSpeed(speed);

    return true;
}

void Soldier::Update(float deltaTime)
{
    if (character != nullptr && currentState != SoldierStates::PATROL)
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    Character::Update(deltaTime);
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
    if (!animComponent) return;

    switch (currentState)
    {
    case SoldierStates::PATROL:
        // GLOG("Soldier Patrolling");
        animComponent->UseTrigger("idle");
        PatrolAI();
        break;
    case SoldierStates::CHASE:
        // GLOG("Soldier Chasing");
        animComponent->UseTrigger("Run");
        ChaseAI();
        break;
    case SoldierStates::BASIC_ATTACK:
        // GLOG("Soldier Basic Attack");
        animComponent->UseTrigger("attack");
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
    if (CheckDistanceWithPlayer() == MEDIUM) currentState = SoldierStates::CHASE;
    else if (CheckDistanceWithPlayer() == CLOSE) currentState = SoldierStates::BASIC_ATTACK;
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
