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

    // Get the arrow. This assumes the archer has only one sibling, which is the arrow. If it has more probably will
    // keep working as long as the arrow is the second gameObject
    const GameObject* root           = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    const std::vector<UID>& siblings = root->GetChildren();
    GameObject* arrowObj             = nullptr;
    for (UID objectUID : siblings)
    {
        if (objectUID != parent->GetUID())
            arrowObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(objectUID);
    }

    if (arrowObj && arrowObj->GetComponent<ScriptComponent*>())
    {
        arrow = arrowObj->GetComponent<ScriptComponent*>()->GetScriptByType<Projectile>();
        if (!arrow) GLOG("[WARNING] No arrow found in archer");
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
    if (!animComponent) return;

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
    animComponent->UseTrigger("run");

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
    animComponent->UseTrigger("run");

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

            if (CheckDistanceWithPlayer() != PlayerDistances::Medium) currentState = ArcherStates::CHASE;
        }
    }
}