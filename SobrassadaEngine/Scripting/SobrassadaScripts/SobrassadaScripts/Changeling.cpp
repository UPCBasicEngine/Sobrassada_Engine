#include "pch.h"

#include "Application.h"
#include "Changeling.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
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
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Changeling)
{
    fields.push_back({"Dash Distance", InspectorField::FieldType::Float, &dashDistance, 0.0f, 10.0f});
    fields.push_back({"Dark Path Name", InspectorField::FieldType::InputText, &pathName});
    fields.push_back({"Body mesh", InspectorField::FieldType::InputText, &bodyMeshPath});
}

bool Changeling::Init()
{
    // GLOG("Initiating Soldier");

    currentState = ChangelingStates::HIDDEN;

    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Archer")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    for (UID childUID : parent->GetChildren())
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child != nullptr)
        {
            if (child->GetName() == pathName)
            {
                dashAreaObject = child;
            } else if (child->GetName() == bodyMeshPath)
            {
                bodyMeshObject = child;
            }
        }
    }
    
    dashAreaObject->GetComponent<CapsuleColliderComponent*>()->centerRotation.x = 1.5708f;
    if (bodyMeshObject != nullptr)
        bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));

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

    /*if (CheckDistanceWithPoint(dashEndPoint) && isDashing)
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
            UID trailPrefabUID = dashAreaObject->GetPrefabUID();

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

            dashAreaObject->SetLocalTransform(trs);
            dashAreaObject->SetLocalPosition(midPoint);

            if (angle < 0.0f) dashAreaObject->GetComponent<CapsuleColliderComponent*>()->centerRotation.y = angle + 1.5708f;
            else dashAreaObject->GetComponent<CapsuleColliderComponent*>()->centerRotation.y = angle - 1.5708f;
            dashAreaObject->GetComponent<CapsuleColliderComponent*>()->length = length;
            AppEngine->GetPhysicsModule()->UpdateCapsuleRigidBody(dashAreaObject->GetComponent<CapsuleColliderComponent*>());

            lastTrailPos = currentPos;
        }
    }*/

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life      = "Health: " + std::to_string(currentHealth);
        const std::string animState = "Anim state: " + stateName.GetString();

        std::vector<std::pair<std::string, float2>> logs {
            {life,      float2(-50.0f, -140.0f)},
            {animState, float2(-80.0f, -160.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
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

void Changeling::UpdateHiddenState(float deltaTime)
{
}

void Changeling::HandleState(float deltaTime)
{
    // if (!animComponent) return;

    switch (currentState)
    {
    case ChangelingStates::HIDDEN:
        // GLOG("Soldier Patrolling");

        UpdateHiddenState(deltaTime);
        HiddenManagement();
        break;
    case ChangelingStates::CHASE:
        // GLOG("Soldier Chasing");
        agentAI->ResetSpeed();
        ChaseAI();
        break;
    case ChangelingStates::DASH_ATTACK:
        // GLOG("Soldier Basic Attack");
        if (attackCdTimer <= 0) Attack(deltaTime);
        break;
    case ChangelingStates::NONE:
        currentState = ChangelingStates::HIDDEN;
        break;
    default:
        GLOG("No state provided to Archer");
        currentState = ChangelingStates::NONE;
        break;
    }

    // if (animComponent && animComponent->IsFinished())
    //{
    //     // GLOG("FINISH ANIM");
    //     animComponent->UseTrigger("idle");
    // }
}

void Changeling::HiddenManagement()
{
    // animComponent->UseTrigger("run");

    if (CheckDistanceWithPlayer() == PlayerDistances::Medium)
    {
        if (bodyMeshObject != nullptr)
            bodyMeshObject->SetLocalPosition(float3(0, 0, 0));
        currentState = ChangelingStates::CHASE;
    }
}

void Changeling::ChaseAI()
{
    //  animComponent->UseTrigger("run");
    if (character != nullptr)
    {
        if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentState = ChangelingStates::DASH_ATTACK;
        else
        {
            isDashing = false;
            if (!agentAI->SetPathNavigation(character->GetLastPosition()))
            {
                if (bodyMeshObject != nullptr)
                    bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));
                agentAI->ResetSpeed();
                currentState = ChangelingStates::HIDDEN;
            }
            ChangeState();
        }
    }
    else currentState = ChangelingStates::NONE;
}

void Changeling::Attack(float deltaTime)
{
    if (!dashAreaObject) return;

    if (!isAttacking)
    {
        float3 direction = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
        direction.Normalize();

        float testDistance = dashDistance;
        float3 dashTarget;
        bool posOverPoly        = false;
        float3 closestPoint     = float3::zero;
        const float3 searchArea = {1.0f, 2.0f, 1.0f};

        // Busca el primer punto v�lido en la navmesh reduciendo la distancia
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
        startPos       = parent->GetPosition();
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

            dashEndPoint = dashDirection;
            agentAI->SetPathNavigation(dashEndPoint);
            dashAreaObject->GetComponent<CapsuleColliderComponent*>()->SetEnabled(true);
        }
    }
}

void Changeling::ChangeState()
{
    const float distance = GetDistanceFromPlayer();
    if (playerScript->IsDead() || distance > maxDetectionRange )
    {
        if (bodyMeshObject != nullptr)
            bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));
        agentAI->ResetSpeed();
        currentState = ChangelingStates::HIDDEN;
    }
}