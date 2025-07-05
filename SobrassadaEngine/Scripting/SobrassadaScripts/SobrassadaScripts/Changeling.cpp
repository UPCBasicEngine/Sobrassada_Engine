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
    fields.push_back({"Dark Path Name", InspectorField::FieldType::InputText, &pathName});
    fields.push_back({"Body mesh", InspectorField::FieldType::InputText, &bodyMeshPath});
    fields.push_back({"Abs spotted reaction time", InspectorField::FieldType::Float, &absoluteSpottedReactionTime, 0.1f, 10.0f});
    fields.push_back({"Abs rise duration", InspectorField::FieldType::Float, &absoluteRiseDuration, 0.1f, 10.0f});
    fields.push_back({"Version", InspectorField::FieldType::Int, &userSelectedVersion, 0, 3});
}

bool Changeling::Init()
{
    // GLOG("Initiating Soldier");

    currentState = ChangelingStates::HIDDEN;

    Character::Init();

    userSelectedVersion = min(max(userSelectedVersion, 0), 3);
    version = static_cast<ChangelingVersions>(userSelectedVersion == 0 ? rand() % 3 + 1 : userSelectedVersion);

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

    isAttacking                                                          = false;
    attackCdTimer                                                        = attackCooldown;
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    characterCollider->SetEnabled(false);

    return true;
}

void Changeling::Update(float deltaTime)
{
    if (agentAI == nullptr) return;
    Character::Update(deltaTime);

    RenderDebugVisuals();
}

void Changeling::OnDeath()
{
    stateTimer = dyingDuration;
    isDead = false; // TODO To keep getting updates until the death animation is finished
    currentState = ChangelingStates::DYING;
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
    float distanceToPlayerSq = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());

    switch (currentState)
    {
    case ChangelingStates::HIDDEN:
        UpdateHiddenState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DIG_UP_TRANSITION:
        UpdateDigUpTransitionState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DIG_DOWN_TRANSITION:
        UpdateDigDownTransitionState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::CHASE:
        UpdateChaseState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DASH_ATTACK_PREPARATION:
        UpdateDashAttackPreparationState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DASH_ATTACK:
        UpdateDashAttackState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DASH_ATTACK_COOLDOWN:
        UpdateDashAttackCooldownState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::BITE_ATTACK:
        UpdateBiteAttackState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::BITE_ATTACK_COOLDOWN:
        UpdateBiteAttackCooldownState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DYING:
        UpdateDyingState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::NONE:
        currentState = ChangelingStates::HIDDEN;
        break;
    }

    stateTimer -= deltaTime;

    // if (animComponent && animComponent->IsFinished())
    //{
    //     // GLOG("FINISH ANIM");
    //     animComponent->UseTrigger("idle");
    // }
}

void Changeling::UpdateHiddenState(float deltaTime, float distanceToPlayerSq)
{
    if (ST_BiteAttack(deltaTime, distanceToPlayerSq)) return; // Preconditions are checked in the function
    
    if (distanceToPlayerSq < rangeAIChase * rangeAIChase)
    {
        if (hasPlayerSpotted)
        {
            stateTimer -= deltaTime;
            if (stateTimer < absoluteRiseDuration)
            {
                characterCollider->SetEnabled(true);
                currentState = ChangelingStates::DIG_UP_TRANSITION;
                // TODO Play animation to dig up
            }

            // Only flower should be lit up
        } else
        {
            hasPlayerSpotted = true;
            stateTimer = absoluteSpottedReactionTime + absoluteRiseDuration;
        }
    } else if (hasPlayerSpotted)
        hasPlayerSpotted = false;
}

void Changeling::UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
    {
        if (stateTimer < 0.f)
        {
            if (bodyMeshObject != nullptr)
                bodyMeshObject->SetLocalPosition(float3(0, 0, 0));
            currentState = ChangelingStates::CHASE;
        } else
        {
            if (bodyMeshObject != nullptr)
                bodyMeshObject->SetLocalPosition(Lerp(float3(0, -1.2f,0 ), float3(0, 0, 0), 1.f - stateTimer / absoluteRiseDuration));
        }
    } else
    {
        hasPlayerSpotted = false;
        stateTimer = absoluteRiseDuration - stateTimer;
        currentState = ChangelingStates::DIG_DOWN_TRANSITION;
    }
}

void Changeling::UpdateDigDownTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (distanceToPlayerSq > rangeAIChase * rangeAIChase)
    {
        if (stateTimer < 0.f)
        {
            if (bodyMeshObject != nullptr)
                bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));
            characterCollider->SetEnabled(false);
            currentState = ChangelingStates::HIDDEN;
        } else
        {
            if (bodyMeshObject != nullptr)
                bodyMeshObject->SetLocalPosition(Lerp(float3(0, -1.2f,0 ), float3(0, 0, 0), stateTimer / absoluteRiseDuration));
        }
    } else
    {
        hasPlayerSpotted = true;
        stateTimer = absoluteRiseDuration - stateTimer;
        currentState = ChangelingStates::DIG_UP_TRANSITION;
    }
}

void Changeling::UpdateChaseState(float deltaTime, float distanceToPlayerSq)
{
    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;
    
    if (playerScript->IsDead() || distanceToPlayerSq > rangeAIChase * rangeAIChase)
    {
        stateTimer = absoluteRiseDuration;
        agentAI->ResetSpeed();
        currentState = ChangelingStates::DIG_DOWN_TRANSITION;
    }
}

void Changeling::UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        Character::Attack(deltaTime);
    
        agentAI->SetSpeed(dashSpeed, 1000000);
        agentAI->SetPathNavigation(dashTarget);
        stateTimer = attackDuration;
        
        weaponCollider->SetEnabled(true);
        currentState = ChangelingStates::DASH_ATTACK;
    }
}

void Changeling::UpdateDashAttackState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        weaponCollider->SetEnabled(false);
        isAttacking = false;
        stateTimer = attackCooldown;
        currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
    }
}

void Changeling::UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;
        stateTimer = absoluteRiseDuration;
        agentAI->ResetSpeed();
        currentState = ChangelingStates::DIG_DOWN_TRANSITION;
    }
}

void Changeling::UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        if (bodyMeshObject != nullptr)
            bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));
        stateTimer = biteAttackCooldown;
        characterCollider->SetEnabled(false);
        weaponCollider->SetEnabled(false);
        currentState = ChangelingStates::BITE_ATTACK_COOLDOWN;
    }
}

void Changeling::UpdateBiteAttackCooldownState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f && !ST_BiteAttack(deltaTime, distanceToPlayerSq))
    {
        hasPlayerSpotted = false;
        currentState = ChangelingStates::HIDDEN;
    }
}

void Changeling::UpdateDyingState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        isDead = true;
        parent->SetEnabled(false);
    } else
    {
        if (bodyMeshObject != nullptr)
            bodyMeshObject->SetLocalPosition(Lerp(float3(0, -1.7f,0 ), float3(0, 0, 0), stateTimer / dyingDuration));
    }
}

bool Changeling::ST_DashAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (distanceToPlayerSq > rangeAIAttack * rangeAIAttack)   return false;
    if (currentState != ChangelingStates::CHASE && currentState != ChangelingStates::DASH_ATTACK_COOLDOWN) return false;
    
    // Implement state transition
    dashDirection = character->GetLastPosition() - parent->GetGlobalTransform().TranslatePart();
    dashDirection.Normalize();
    dashDirection.y = 0;

    float3 targetPoint;
    float3 resultPos;
    float testDistance = rangeAIAttack;
    bool posOverPoly        = false;
    const float3 searchArea = {1.0f, 1.0f, 1.0f};

    do
    {
        targetPoint = parent->GetGlobalTransform().TranslatePart() + dashDirection * testDistance;
        agentAI->GetClosestPointInNavmesh(targetPoint, searchArea, posOverPoly, resultPos);
        if (!posOverPoly) testDistance -= 1;
    } while (!posOverPoly && testDistance > 0.0f);

    if (!posOverPoly) return false;
    
    dashTarget = targetPoint;
    currentState = ChangelingStates::DASH_ATTACK_PREPARATION;

    agentAI->SetLookForward(false);
    agentAI->SetSpeed(0.0f, 0.0f);
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    agentAI->LookAtMovement(dashTarget, deltaTime);
    
    stateTimer = dashAttackPreparationDuration;

    return true;
}

bool Changeling::ST_BiteAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (distanceToPlayerSq > biteAttackRadius * biteAttackRadius)   return false;
    if (currentState != ChangelingStates::HIDDEN && currentState != ChangelingStates::BITE_ATTACK_COOLDOWN) return false;
    
    // Implement state transition
    if (bodyMeshObject != nullptr)
        bodyMeshObject->SetLocalPosition(float3(0, -.8f, 0));
    characterCollider->SetEnabled(true);
    hasPlayerSpotted = true;
    stateTimer = biteAttackDuration;
    currentState = ChangelingStates::BITE_ATTACK;

    weaponCollider->SetEnabled(true);

    Character::Attack(deltaTime);

    return true;
}

void Changeling::RenderDebugVisuals()
{
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
