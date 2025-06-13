#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

Banshee::Banshee(GameObject* parent)
    : Character(
          parent,
          2,     // Max Health
          2,     // Damage
          2.0f,  // Attack Duration
          4.0f,  // Attack Cooldown
          5.0f,  // Attack Range
          5.0f,  // AI Aggro Range
          5.0f,  // AI Chase Range
          10.0f, // Max detection range
          CharacterType::Banshee
      )
{
    fields.push_back({"Fleeing Distance", InspectorField::FieldType::Float, &fleeDistance, 0.0f, 10.0f});
    fields.push_back({"Fleeing Speed", InspectorField::FieldType::Float, &fleeSpeed, 0.0f, 10.0f});
    fields.push_back({"Attack Angular Speed", InspectorField::FieldType::Float, &attackAngularSpeed, 0.0f, 10.0f});
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

    if (weapon)
    {
        damageArea = weapon->GetComponent<SphereColliderComponent*>();
        if (damageArea == nullptr) GLOG("Sphere collider not found for Banshee")
        else damageArea->SetEnabled(false);
    }

    if (parent->GetChildren().size() > 3)
    {
        areaVisual = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[3]);
        if (areaVisual) areaVisual->SetEnabled(false);
        else GLOG("[WARNING] Banshee: no area visual found as child of base")
    }

    if (parent->GetChildren().size() > 4)
    {
        screamVisual = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[4]);
        if (screamVisual) screamVisual->SetEnabled(false);
        else GLOG("[WARNING] Banshee: no scream visual found as child of base")
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
    parent->SetEnabled(false);
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

    case BansheeStates::Search:
        SearchForPlayer();
        break;

    case BansheeStates::Chase:
        ChasePlayer();
        break;

    case BansheeStates::Flee:
        Flee();
        break;

    case BansheeStates::Scream:
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    }
}

void Banshee::ChasePlayer()
{
    if (!character) return;

    if (animComponent) animComponent->UseTrigger("Chase");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close) currentState = BansheeStates::Scream;
    else if (!agentAI->SetPathNavigation(character->GetLastPosition()) || GetDistanceFromPlayer() > maxDetectionRange)
        currentState = BansheeStates::Search;
}

void Banshee::Flee()
{
    if (!isFleeing)
    {
        if (animComponent) animComponent->UseTrigger("Chase");
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

void Banshee::Attack(float deltaTime)
{
    if (!damageArea) return;

    if (!isAttacking)
    {
        GLOG("Banshee attack");
        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("Scream");

        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);
        agentAI->SetAngularSpeed(attackAngularSpeed);
    }
    else
    {
        // Slowly rotate towards player while charging the attack
        if (attackTimer < attackHitboxDelay) agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!damageArea->GetEnabled() && attackTimer >= attackHitboxDelay &&
            attackTimer <= attackHitboxDelay + attackHitboxDuration)
        {
            GLOG("Banshee enable hitbox");
            if (areaVisual) areaVisual->SetEnabled(true);
            if (screamVisual) screamVisual->SetEnabled(true);
            damageArea->SetEnabled(true);
            if (weaponCollider) weaponCollider->SetEnabled(true);
        }
        else if (damageArea->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
        {
            GLOG("Banshee disable hitbox");
            damageArea->SetEnabled(false);
            if (weaponCollider) weaponCollider->SetEnabled(false);
            if (areaVisual) areaVisual->SetEnabled(false);
            if (screamVisual) screamVisual->SetEnabled(false);
        }

        if (attackTimer >= attackDuration)
        {
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->ResetAngularSpeed();
            agentAI->SetLookForward(true);
            ChangeState();
        }
    }
}

void Banshee::ChangeState()
{
    const float distance = GetDistanceFromPlayer();
    if (distance <= fleeDistance) currentState = BansheeStates::Flee;
    else if (distance <= rangeAIAttack) currentState = BansheeStates::Scream;
    else if (distance <= rangeAIChase) currentState = BansheeStates::Chase;
    else currentState = BansheeStates::Idle;
}

void Banshee::SearchForPlayer()
{
    // Stands still for a few seconds, if player gets close again chases, if not returns to patrol
    if (!isSearching)
    {
        // TODO: Would be nice to be a "search" animation instead of idle
        animComponent->UseTrigger("Idle");
        isSearching = true;
        searchTimer = searchDuration;
        agentAI->SetSpeed(0.0f, 0.0f);
    }

    if (GetDistanceFromPlayer() < maxDetectionRange - 0.5f)
    {
        isSearching = false;
        agentAI->ResetSpeed();
        currentState = BansheeStates::Chase;
    }
    else if (searchTimer <= 0.0f)
    {
        // TODO: When merge with new banshee behaviour, teleport to start pos
        isSearching  = false;
        currentState = BansheeStates::Idle;
        agentAI->ResetSpeed();
        agentAI->SetPathNavigation(startPos);
    }
}