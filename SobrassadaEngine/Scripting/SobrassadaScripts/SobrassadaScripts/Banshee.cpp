#include "pch.h"

#include "Banshee.h"

#include "GameObject.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"

Banshee::Banshee(GameObject* parent)
    : Character(
          parent,
          1, // Max Health
          2, // Damage
          3, // Attack Duration
          4, // Attack Cooldown
          5, // Attack Range
          5, // AI Aggro Range
          5, // AI Chase Range
          CharacterType::Banshee
      )
{
}

bool Banshee::Init()
{
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

void Banshee::Update(float deltaTime)
{
    Character::Update(deltaTime);
}

void Banshee::OnDeath()
{
}

void Banshee::OnDamageTaken(int amount)
{
}

void Banshee::PerformAttack()
{
}

void Banshee::HandleState()
{
    switch (currentState)
    {
    case BansheeStates::Idle:
        break;

    case BansheeStates::Chase:
        ChasePlayer();
        break;

    case BansheeStates::Flee:
        Flee();
        break;

    case BansheeStates::Scream:
        Attack();
        break;
    }
}

void Banshee::ChasePlayer()
{
}

void Banshee::Flee()
{
}

void Banshee::Attack()
{
    if (!isAttacking)
    {
        GLOG("Banshee attack");
        animComponent->UseTrigger("Tal");
        isAttacking = true;
        agentAI->PauseMovement();
    }
    else
    {
        // Enable hitbox when animation
    }
}