#include "pch.h"

#include "Application.h"
#include "Changeling.h"
#include "Component.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "Math/Quat.h"
#include "PhysicsModule.h"
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
    fields.push_back({"Dash Distance", InspectorField::FieldType::Float, &dashDistance, 0.0f, 10.0f});
    fields.push_back({"Dark Path Name", InspectorField::FieldType::InputText, &pathName});
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

    pathObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(pathName);
    pathObj->GetComponent<CapsuleColliderComponent*>()->centerRotation.x = 1.5708f;

    hasShot                                                              = false;
    isAttacking                                                          = false;
    attackCdTimer                                                        = attackCooldown;
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    return true;
}

void Changeling::Update(float deltaTime)
{
    if (agentAI == nullptr) return;
    Character::Update(deltaTime);

    if (CheckDistanceWithPoint(dashEndPoint) && isDashing)
    {
        // agentAI->PauseMovement();
        agentAI->SetSpeed(0.0f, 0.0f);
        isDashing = false;
        hasShot   = true;
        if (CheckDistanceWithPlayer() != PlayerDistances::Close) currentState = ChangelingStates::CHASE;

        hasShot       = false;
        isAttacking   = false;
        attackCdTimer = attackCooldown;
        // agentAI->ResetSpeed();
        // agentAI->SetLookForward(true);
    }

    if (isDashing)
    {
        float3 currentPos = parent->GetPosition();
        float3 scale      = float3::one;
        float4x4 trs      = float4x4::identity;
        float3 position   = float3::zero;
        if ((currentPos - lastTrailPos).Length() >= trailSegmentSpacing)
        {
            UID trailPrefabUID = pathObj->GetPrefabUID();

            float3 position    = currentPos;

            float3 midPoint    = (startPos + position) * 0.5f;

            float3 direction   = (position - startPos).Normalized();

            localTransform     = parent->GetLocalTransform();
            float3 forward     = parent->GetGlobalTransform().WorldZ();
            forward.y          = 0.0f;
            forward.Normalize();

            float length  = (position - startPos).Length();

            scale         = float3(1, 1, length);
            float angle   = atan2(direction.x, direction.z);
            Quat rotation = Quat::FromEulerXYZ(0.0f, angle, 0.0f);

            trs           = float4x4::FromTRS(midPoint, rotation, scale);

            pathObj->SetLocalTransform(trs);
            pathObj->SetLocalPosition(midPoint);

            if (angle < 0.0f) pathObj->GetComponent<CapsuleColliderComponent*>()->centerRotation.y = angle + 1.5708f;
            else pathObj->GetComponent<CapsuleColliderComponent*>()->centerRotation.y = angle - 1.5708f;
            pathObj->GetComponent<CapsuleColliderComponent*>()->length = length;
            AppEngine->GetPhysicsModule()->UpdateCapsuleRigidBody(pathObj->GetComponent<CapsuleColliderComponent*>());

            lastTrailPos = currentPos;
        }
    }
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
    // if (!animComponent) return;

    switch (currentState)
    {
    case ChangelingStates::PATROL:
        // GLOG("Soldier Patrolling");
        PatrolAI();
        break;
    case ChangelingStates::CHASE:
        // GLOG("Soldier Chasing");
        agentAI->ResetSpeed();
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

    // if (animComponent && animComponent->IsFinished())
    //{
    //     // GLOG("FINISH ANIM");
    //     animComponent->UseTrigger("idle");
    // }
}

void Changeling::PatrolAI()
{
    // animComponent->UseTrigger("run");

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
    // if (isDashing) return;
    //  animComponent->UseTrigger("run");
    if (character != nullptr)
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentState = ChangelingStates::BASIC_ATTACK;
        else
        {
            isDashing = false;
            if (!agentAI->SetPathNavigation(character->GetLastPosition())) currentState = ChangelingStates::PATROL;
        }
    }
    else currentState = ChangelingStates::PATROL;
}

void Changeling::Attack(float deltaTime)
{
    if (!pathObj) return;

    if (!isAttacking)
    {
        GLOG("ATTACK ENEMY");
        float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
        direction.Normalize();

        float testDistance = dashDistance;
        float3 dashTarget;
        bool posOverPoly    = false;
        float3 closestPoint = float3::zero;
        float3 searchArea   = {1.0f, 2.0f, 1.0f};

        // Busca el primer punto válido en la navmesh reduciendo la distancia
        do
        {
            dashDirection = parent->GetGlobalTransform().TranslatePart() + direction * testDistance;
            agentAI->GetClosestPointInNavmesh(dashDirection, searchArea, posOverPoly, closestPoint);
            if (!posOverPoly) testDistance -= 1.0f;
        } while (!posOverPoly && testDistance > 0.0f);

        dashDirection = posOverPoly ? closestPoint : character->GetLastPosition();

        agentAI->SetLookForward(false);
        if (animComponent) animComponent->UseTrigger("attack");
        Character::Attack(deltaTime);
        agentAI->SetSpeed(0.0f, 0.0f);
        startPos = parent->GetPosition();
        GLOG("startPos: x=%.3f, y=%.3f, z=%.3f", startPos.x, startPos.y, startPos.z);
        localTransform = parent->GetLocalTransform();

        hasShot        = false;
        attackCdTimer  = attackCooldown;
        agentAI->ResetSpeed();
        agentAI->SetLookForward(true);
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        // Enable hitbox when animation hits
        if (!hasShot && attackTimer >= attackHitboxDelay)
        {
            lastTrailPos = parent->GetPosition();
            hasShot      = true;
            isDashing    = true;
            agentAI->SetSpeed(dashSpeed, 1000000);

            dashEndPoint        = dashDirection;
            agentAI->SetPathNavigation(dashEndPoint);
            pathObj->GetComponent<CapsuleColliderComponent*>()->SetEnabled(true);
        }
    }
}