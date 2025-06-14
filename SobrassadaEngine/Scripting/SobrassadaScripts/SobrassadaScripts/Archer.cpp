#include "pch.h"

#include "Application.h"
#include "Archer.h"
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

Archer::Archer(GameObject* parent) : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, CharacterType::Archer)
{
    fields.push_back({"AI Patrol Point", InspectorField::FieldType::Vec3, &patrolPoint, -1000.0f, 1000.0f});
    fields.push_back({"Arrow Projectile Name", InspectorField::FieldType::InputText, &arrowName});
}

bool Archer::Init()
{
    // GLOG("Initiating Soldier");

    currentState = ArcherStates::PATROL;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Archer")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    const GameObject* arrowObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(arrowName);
    if (arrowObj && arrowObj->GetComponent<ScriptComponent*>())
    {
        arrow = arrowObj->GetComponent<ScriptComponent*>()->GetScriptByType<Projectile>();
        if (!arrow) GLOG("[WARNING] No projectile found by the name %s", arrowName.c_str());
    }

    return true;
}

void Archer::Update(float deltaTime)
{
    if (agentAI == nullptr) return;
    Character::Update(deltaTime);
}

void Archer::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
    parent->SetEnabled(false);
}

void Archer::OnDamageTaken(int amount)
{
    // TODO: play soldier take damage sound
    // TODO: particles? and animation
}

void Archer::PerformAttack()
{
    // TODO: play basicAttack sound
    // TODO: make interaction with hitboxes with the character
    // TODO: activate and disable the box collider located on one on the gameobjects weapon
    // TODO: trails, particles and animation
}

void Archer::HandleState(float deltaTime)
{
    // if (!animComponent) return;

    switch (currentState)
    {
    case ArcherStates::PATROL:
        // GLOG("Soldier Patrolling");
        PatrolAI();
        break;
    case ArcherStates::CHASE:
        // GLOG("Soldier Chasing");
        ChaseAI();
        break;
    case ArcherStates::BASIC_ATTACK:
        // GLOG("Soldier Basic Attack");
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    case ArcherStates::ESCAPE:
        Escape(deltaTime);
        break;
    default:
        GLOG("No state provided to Archer");
        currentState = ArcherStates::PATROL;
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        // GLOG("FINISH ANIM");
        animComponent->UseTrigger("idle");
    }
}

void Archer::PatrolAI()
{
    if (animComponent) animComponent->UseTrigger("run");

    if (CheckDistanceWithPlayer() == PlayerDistances::Medium) currentState = ArcherStates::CHASE;
    else if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentState = ArcherStates::BASIC_ATTACK;

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

void Archer::ChaseAI()
{
    if (animComponent) animComponent->UseTrigger("run");

    if (character != nullptr)
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Medium) currentState = ArcherStates::BASIC_ATTACK;
        else if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = ArcherStates::PATROL;
    }
    else currentState = ArcherStates::PATROL;
}

void Archer::Attack(float deltaTime)
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
            hasShot          = true;
            float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
            direction.Normalize();
            arrow->Shoot(parent->GetPosition(), direction);
        }

        // Reset attack state
        if (attackTimer >= attackDuration)
        {
            hasShot       = false;
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResetSpeed();
            agentAI->SetLookForward(true);

            if (character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart()) <= rangeAIChase / 2)
            {
                currentState = ArcherStates::ESCAPE;
            }
            else if (CheckDistanceWithPlayer() >= PlayerDistances::Medium) currentState = ArcherStates::CHASE;
        }
    }
}

void Archer::Escape(float deltaTime)
{
    GLOG("Archer Escaping");
    if (!agentAI || !character) return;

    float3 archerPos = parent->GetGlobalTransform().TranslatePart();
    float3 playerPos = character->GetLastPosition();

    float3 escapeDir = archerPos - playerPos;
    escapeDir.Normalize();

    float escapeDistance = rangeAIChase;
    float3 escapeTarget  = archerPos + escapeDir * escapeDistance;

    float3 searchArea    = {1.0f, 2.0f, 1.0f}; 
    bool posOverPoly     = false;
    float3 closestPoint  = float3::zero;
    float3 navmeshTarget = escapeTarget;

    agentAI->GetClosestPointInNavmesh(escapeTarget, searchArea, posOverPoly, closestPoint);
    if (posOverPoly) navmeshTarget = closestPoint;
    else navmeshTarget = archerPos; 

    agentAI->SetPathNavigation(navmeshTarget);
    agentAI->LookAtMovement(navmeshTarget, deltaTime);

    if (animComponent) animComponent->UseTrigger("run");

    if (CheckDistanceWithPlayer() > PlayerDistances::Medium)
    {
        currentState = ArcherStates::BASIC_ATTACK;
    }
}
