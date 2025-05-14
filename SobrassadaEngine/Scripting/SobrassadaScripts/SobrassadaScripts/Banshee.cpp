#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"

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
    fields.push_back({"Fleeing distance", InspectorField::FieldType::Float, &fleeDistance, 0.0f, 10.0f});
    fields.push_back({"Fleeing speed", InspectorField::FieldType::Float, &fleeSpeed, 0.0f, 10.0f});
}

bool Banshee::Init()
{
    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Banshee")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    return true;
}

void Banshee::Update(float deltaTime)
{
    if (agentAI == nullptr) return;

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

void Banshee::HandleState(float deltaTime)
{
    switch (currentState)
    {
    case BansheeStates::Idle:
        if (animComponent) animComponent->UseTrigger("Idle");
        ChangeState();
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
    if (!character) return;

    GLOG("CHASE");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close) currentState = BansheeStates::Scream;
    else if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = BansheeStates::Idle;
}

void Banshee::Flee()
{
    GLOG("FLEE");

    // TODO: Could be interesting to increase its speed when flee
    // The commented lines must be uncommented, bu there is a crash when adding new engine functions to the scripts (and
    // are declared in the .cpp, in .h work)

    if (!isFleeing)
    {
        isFleeing = true;
        // agentAI->SetSpeed(fleeSpeed);
    }

    const float3 newPos =
        (parent->GetGlobalTransform().TranslatePart() - character->GetGlobalTransform().TranslatePart()).Normalized() +
        parent->GetGlobalTransform().TranslatePart();

    agentAI->SetPathNavigation(newPos);

    const float distance = character->GetLastPosition().Distance(parent->GetPosition());
    if (distance > fleeDistance)
    {
        isFleeing    = false;
        // agentAI->ResetSpeed();
        currentState = BansheeStates::Scream;
    }
}

void Banshee::Attack()
{
    if (!isAttacking)
    {
        GLOG("Banshee attack");
        if (animComponent) animComponent->UseTrigger("Attack");

        Character::Attack();
        agentAI->PauseMovement();
    }
    else
    {
        // Enable hitbox when animation

        if (attackTimer <= 0)
        {
            isAttacking = false;
            agentAI->ResumeMovement();
            ChangeState();
        }
    }
}

void Banshee::ChangeState()
{
    const float distance = character->GetLastPosition().Distance(parent->GetPosition());
    if (distance <= fleeDistance) currentState = BansheeStates::Flee;
    else if (distance <= rangeAIAttack) currentState = BansheeStates::Scream;
    else if (distance <= rangeAIChase) currentState = BansheeStates::Chase;
    else currentState = BansheeStates::Idle;
}