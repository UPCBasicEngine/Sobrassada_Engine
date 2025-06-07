#include "pch.h"

#include "Banshee.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
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
    fields.push_back({"Invisible time range", InspectorField::FieldType::Vec2, &invisibleTimeRange, 0.0f, 10.0f});
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

    mesh = parent->GetComponentChild<MeshComponent*>(AppEngine);
    if (!mesh) GLOG("No mesh found for Banshee");

    rng            = std::mt19937(std::random_device {}());
    normalizedDist = std::uniform_real_distribution<float>(0.0f, 1.0f);
    invisibleDist  = std::uniform_real_distribution<float>(invisibleTimeRange[0], invisibleTimeRange[1]);

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

    case BansheeStates::Chase:
        ChasePlayer();
        break;

    case BansheeStates::Attack:
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        animComponent->UseTrigger("Idle");
    }
}

void Banshee::ChasePlayer()
{
    if (!character) return;

    if (animComponent) animComponent->UseTrigger("Chase");
    if (CheckDistanceWithPlayer() <= PlayerDistances::Close) currentState = BansheeStates::Attack;
    else if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = BansheeStates::Idle;
}

void Banshee::Attack(float deltaTime)
{
    if (!damageArea) return;

    if (!isAttacking)
    {
        GLOG("Banshee attack");
        agentAI->SetLookForward(false);

        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);

        currentInvisibleTime = invisibleDist(rng);
        GLOG("Current inivivible time: %f", currentInvisibleTime);
        isInvisible = true;
        mesh->SetEnabled(false);
    }
    else
    {
        if (attackTimer < currentInvisibleTime) return;

        if (isInvisible)
        {
            // Tp to player and enable
            GetAttackPosition();
            mesh->SetEnabled(true);
            isInvisible = false;
            agentAI->SetAngularSpeed(attackAngularSpeed);
            if (animComponent) animComponent->UseTrigger("Scream");
        }

        // Slowly rotate towards player while charging the attack
        if (attackTimer < currentInvisibleTime + attackHitboxDelay)
            agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (!damageArea->GetEnabled() && attackTimer >= currentInvisibleTime + attackHitboxDelay &&
            attackTimer <= currentInvisibleTime + attackHitboxDelay + attackHitboxDuration)
        {
            GLOG("Banshee enable hitbox");
            if (areaVisual) areaVisual->SetEnabled(true);
            if (screamVisual) screamVisual->SetEnabled(true);
            damageArea->SetEnabled(true);
            if (weaponCollider) weaponCollider->SetEnabled(true);
        }
        else if (damageArea->GetEnabled() &&
                 attackTimer >= currentInvisibleTime + attackHitboxDelay + attackHitboxDuration)
        {
            GLOG("Banshee disable hitbox");
            damageArea->SetEnabled(false);
            if (weaponCollider) weaponCollider->SetEnabled(false);
            if (areaVisual) areaVisual->SetEnabled(false);
            if (screamVisual) screamVisual->SetEnabled(false);
        }

        if (attackTimer >= currentInvisibleTime + attackDuration)
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
    const float distance = character->GetLastPosition().Distance(parent->GetPosition());
    if (distance <= rangeAIAttack) currentState = BansheeStates::Attack;
    else if (distance <= rangeAIChase) currentState = BansheeStates::Chase;
    else currentState = BansheeStates::Idle;
}

void Banshee::GetAttackPosition()
{
    const float3 playerPos = character->GetLastPosition();
    const float maxRadius  = 2.5f;
    const float minRadius  = 1.5f;

    float angle            = normalizedDist(rng) * 2.0f * PI;
    float r = sqrtf(normalizedDist(rng) * (maxRadius * maxRadius - minRadius * minRadius) + minRadius * minRadius);

    float3 position(cosf(angle) * r + playerPos.x, playerPos.y, sinf(angle) * r + playerPos.z);

    agentAI->SetPosition(position);

    agentAI->LookAtMovement(character->GetLastPosition(), 1.0f);
}