#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

Banshee::Banshee(GameObject* parent)
    : Character(
          parent,
          2, // Max Health
          2, // Damage
          2, // Attack Duration
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

    damageArea = parent->GetComponent<SphereColliderComponent*>();
    if (damageArea == nullptr) GLOG("Sphere collider not found for Banshee")
    else damageArea->SetEnabled(false);

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

void Banshee::HandleState()
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
        if (attackCdTimer <= 0) Attack();
        break;
    }
}

void Banshee::ChasePlayer()
{
    if (!character) return;

    if (CheckDistanceWithPlayer() <= PlayerDistances::Close) currentState = BansheeStates::Scream;
    else if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = BansheeStates::Idle;
}

void Banshee::Flee()
{
    if (!isFleeing)
    {
        isFleeing = true;
        agentAI->SetSpeed(fleeSpeed, 100.0f);
    }

    const float3 newPos =
        (parent->GetGlobalTransform().TranslatePart() - character->GetGlobalTransform().TranslatePart()).Normalized() +
        parent->GetGlobalTransform().TranslatePart();

    agentAI->SetPathNavigation(newPos);

    const float distance = character->GetLastPosition().Distance(parent->GetPosition());
    if (attackCdTimer <= 0 && distance > fleeDistance)
    {
        isFleeing = false;
        agentAI->ResetSpeed();
        ChangeState();
    }
}

void Banshee::Attack()
{
    if (!damageArea) return;

    if (!isAttacking)
    {
        GLOG("Banshee attack");
        if (animComponent) animComponent->UseTrigger("Attack");

        Character::Attack();
        damageArea->SetEnabled(true);
        agentAI->PauseMovement();
    }
    else
    {
        // TODO: Enable hitbox when animation (done in V2)

        if (attackTimer <= 0)
        {
            isAttacking = false;
            damageArea->SetEnabled(false);
            attackCdTimer = attackCooldown;
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