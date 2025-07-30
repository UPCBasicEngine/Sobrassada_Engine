#include "pch.h"

#include "Application.h"
#include "Changeling.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "Globals.h"
#include "Projectile.h"
#include "ResourceStateMachine.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

#include <Math/MathFunc.h>
#include <Math/Quat.h>

Changeling::Changeling(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Changeling)
{
    fields.emplace_back("Dash trail mesh", InspectorField::FieldType::InputText, &dashTrailMeshName);
    fields.emplace_back("Dash trail collision", InspectorField::FieldType::InputText, &dashTrailCollisionName);
    
    fields.emplace_back("Abs spotted reaction time", InspectorField::FieldType::Float, &absoluteSpottedReactionTime, 0.1f, 10.0f);

    fields.emplace_back("Bite attack radius", InspectorField::FieldType::Float, &biteAttackRadius, 0.1f, 10.0f);
    fields.emplace_back("Bite attack cooldown", InspectorField::FieldType::Float, &biteAttackCooldown, 0.1f, 10.0f);

    // Version selection (0 random, 1 sepp, 2 herbert, 3 giacomo)
    fields.emplace_back("Version (0: Random)", InspectorField::FieldType::Int, &userSelectedVersion, 0, 2);

    // Herbert specific (Index 1)
    fields.emplace_back("Chase speed", InspectorField::FieldType::Float, &chaseSpeed, 0.1f, 10.0f);
    fields.emplace_back("Chase Acceleration", InspectorField::FieldType::Float, &chaseAcceleration, 0.1f, 10.0f);

    // Sepp specific (Index 2)
    fields.emplace_back("Max sneak angle degrees", InspectorField::FieldType::Float, &maxSneakAngleDegrees, 1.0f, 180.0f);
    fields.emplace_back("Min sneak speed", InspectorField::FieldType::Float, &minSneakSpeed, 0.001f, 10.0f);
    fields.emplace_back("Max sneak speed", InspectorField::FieldType::Float, &maxSneakSpeed, 0.1f, 10.0f);
    fields.emplace_back("Distance to player for max sneak speed", InspectorField::FieldType::Float, &distanceToPlayerForMaxSneakSpeed, 0.1f, 10.0f);
    fields.emplace_back("Sneak Acceleration", InspectorField::FieldType::Float, &sneakAcceleration, 0.1f, 10.0f);
    fields.emplace_back("Peek chance per second", InspectorField::FieldType::Float, &peekChancePerSecond, 0.1f, 10.0f);

    // Giacomo specific (Index 3)
    fields.emplace_back("Dash angle degrees", InspectorField::FieldType::Float, &dashAngleDegrees, 0.0f, 180.0f);
}

bool Changeling::Init()
{
    // GLOG("Initiating Soldier");

    ValidateSetup();

    if (!isSetupCorrectly)
    {
        GLOG("[WARNING] Changeling not setup correctly")
        return false;
    }

    currentState = ChangelingStates::IDLE_BURRIED;

    Character::Init();

    version = static_cast<ChangelingVersions>(userSelectedVersion == 0 ? rand() % 3 + 1 : userSelectedVersion);

    agentAI->RecreateAgent();
    agentAI->SetLookForward(true);
    speed = agentAI->GetSpeed();

    for (auto dashTrailMeshObject : dashTrailMeshObjects)
        dashTrailMeshObject->SetEnabled(false);
    for (auto dashTrailColliderObject : dashTrailColliderObjects)
        dashTrailColliderObject->SetEnabled(false);

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

void Changeling::OnPlayerExitLocation()
{
    // TODO

    //const HashString& playerLocationTag = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    //bool isPlayerInLocation = parent->HasTag(playerLocationTag);
}

void Changeling::OnPlayerEnterLocation()
{
    // TODO
}

void Changeling::OnDeath()
{
    isDead = false; // TODO To keep getting updates until the death animation is finished
    if (animComponent) animComponent->UseTrigger("Trigger_Die");
    agentAI->SetSpeed(0, 10);
    currentState = ChangelingStates::DYING;
}

void Changeling::OnDamageTaken(int amount)
{
    currentState = ChangelingStates::DAMAGED;

    agentAI->ResetSpeed();
    agentAI->SetSpeed(0, 10);

    for (auto dashTrailMeshObject : dashTrailMeshObjects)
        dashTrailMeshObject->SetEnabled(false);
    
    for (auto dashTrailColliderObject : dashTrailColliderObjects)
        dashTrailColliderObject->SetEnabled(false);
    
    if (animComponent)
    {
        if (!animComponent->UseTrigger("Trigger_Hit")) // TODO Randomize different damage animations
            animComponent->UseTrigger("Trigger_HitUnderground");
    }
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
    // Don´t update a changeling with wrong setup
    if (!isSetupCorrectly) return;
    
    float distanceToPlayerSq = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());

    switch (currentState)
    {
    case ChangelingStates::IDLE_BURRIED:
        UpdateIdleBurriedState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::PEEK:
        UpdatePeekState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DIG_UP_TRANSITION:
        UpdateDigUpTransitionState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DIG_DOWN_TRANSITION:
        UpdateDigDownTransitionState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::IDLE_VISIBLE:
        UpdateIdleVisibleState(deltaTime, distanceToPlayerSq);
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
    case ChangelingStates::DASH_CHAIN_ATTACK:
        UpdateDashChainAttackState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::BITE_ATTACK:
        UpdateBiteAttackState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::BITE_ATTACK_COOLDOWN:
        UpdateBiteAttackCooldownState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DAMAGED:
        UpdateDamagedState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DYING:
        UpdateDyingState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::NONE:
        currentState = ChangelingStates::IDLE_BURRIED;
        break;
    }

    stateTimer -= deltaTime;

    // if (animComponent && animComponent->IsFinished())
    //{
    //     // GLOG("FINISH ANIM");
    //     animComponent->UseTrigger("idle");
    // }
}

void Changeling::UpdateIdleBurriedState(float deltaTime, float distanceToPlayerSq)
{
    if (ST_BiteAttack(deltaTime, distanceToPlayerSq)) return; // Preconditions are checked in the function

    if (version == ChangelingVersions::HERBERT)
    {
        const int randomValue = max(1, (int)round(1.0f / (peekChancePerSecond * deltaTime)));
        GLOG("FPS: %f => %d", 1.0f / deltaTime, randomValue);
        // Random value = 1 only if fps are too high -> Ignore those frames then
        if (randomValue != 1 && rand() % randomValue == 0 && ST_Peek(deltaTime, 0)) return;
        
        if (distanceToPlayerSq < maxDetectionRange * maxDetectionRange)
        {
            const float3 directionToPlayer = (character->GetGlobalTransform().TranslatePart() - parent->GetGlobalTransform().TranslatePart()).Normalized();
            const float angleToPlayerVision = character->GetFrontDirection().AngleBetween(directionToPlayer) * RAD_DEGREE_CONV;
            if (angleToPlayerVision < maxSneakAngleDegrees)
            {
                const float lerpFactor = max(min((distanceToPlayerSq - Pow(distanceToPlayerForMaxSneakSpeed, 2)) /
                    Pow(maxDetectionRange - distanceToPlayerForMaxSneakSpeed, 2), 1), 0);
                
                const float currentSneakSpeed = minSneakSpeed + (maxSneakSpeed - minSneakSpeed) * (1 - lerpFactor);

                agentAI->ResumeMovement();
                agentAI->LookAtMovement(character->GetGlobalTransform().TranslatePart(), deltaTime);
                agentAI->SetSpeed(currentSneakSpeed, sneakAcceleration);
                agentAI->SetPathNavigation(character->GetGlobalTransform().TranslatePart());
                
            } else
            {
                agentAI->SetSpeed(0, 10);
            }
        }
    } else
    {
        if (distanceToPlayerSq < rangeAIChase * rangeAIChase)
        {
            // Only start digging up when the reaction time is over
            if (hasPlayerSpotted)
            {
                if (stateTimer < 0)
                {
                    hasPlayerSpotted = false;
                    characterCollider->SetEnabled(true);
                    if (animComponent) animComponent->UseTrigger("Trigger_BuryUp");
                    currentState = ChangelingStates::DIG_UP_TRANSITION;
                }
            } else
            {
                hasPlayerSpotted = true;
                stateTimer = absoluteSpottedReactionTime;
            }
        } else hasPlayerSpotted = false;
    }
    
}

void Changeling::UpdatePeekState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        characterCollider->SetEnabled(false);
        animComponent->UseTrigger("Trigger_BurriedIdle");
        currentState = ChangelingStates::IDLE_BURRIED;
    }
}

void Changeling::UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
    
}

void Changeling::UpdateDigDownTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        animComponent->UseTrigger("Trigger_BurriedIdle");
        currentState = ChangelingStates::IDLE_BURRIED;
    }
}

void Changeling::UpdateIdleVisibleState(float deltaTime, float distanceToPlayerSq)
{
    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

    if (distanceToPlayerSq <= rangeAIAttack * rangeAIAttack)
    {
        if (animComponent) animComponent->UseTrigger("Trigger_PrepareDash");
        currentState = ChangelingStates::DASH_ATTACK_PREPARATION;
    } else if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
    {
        if (animComponent) animComponent->UseTrigger("Trigger_Run");
        currentState = ChangelingStates::CHASE;
    } else 
    {
        if (animComponent) animComponent->UseTrigger("Trigger_BuryDown");
        currentState = ChangelingStates::DIG_DOWN_TRANSITION;
    }
}

void Changeling::UpdateChaseState(float deltaTime, float distanceToPlayerSq)
{
    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;
    
    if (distanceToPlayerSq > rangeAIChase * rangeAIChase)
    {
        agentAI->SetSpeed(0.0f, 10.0f);
        if (animComponent) animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    } else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        agentAI->SetSpeed(chaseSpeed, chaseAcceleration);
        agentAI->SetPathNavigation(character->GetLastPosition());
    }
}

void Changeling::UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq)
{
    agentAI->LookAtMovement(dashTarget, deltaTime);
    
    if (animComponent && animComponent->IsFinished())
    {
        Character::Attack(deltaTime);
    
        agentAI->SetSpeed(dashSpeed, 1000000);
        agentAI->SetPathNavigation(dashTarget);
        stateTimer = attackDuration;
        
        weaponCollider->SetEnabled(true);
        dashTrailMeshObjects[0]->SetEnabled(true);   
        dashTrailColliderObjects[0]->SetEnabled(true);
        if (version == ChangelingVersions::FRANZ)
        {
            dashIndex = 0;
            animComponent->UseTrigger("Trigger_Dash");
            currentState = ChangelingStates::DASH_CHAIN_ATTACK;
        } else
        {
            animComponent->UseTrigger("Trigger_Dash");
            currentState = ChangelingStates::DASH_ATTACK;
        }
    }
}

void Changeling::UpdateDashAttackState(float deltaTime, float distanceToPlayerSq)
{
    // TODO This might need a rework so the dash speed is always the same, even if its cancelled early on the route
    if (stateTimer < 0.f)
    {
        weaponCollider->SetEnabled(false);
        isAttacking = false;
        stateTimer = attackCooldown;
        if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
        currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
    } else {
        float distanceFromDashStart = parent->GetGlobalTransform().TranslatePart().Distance(dashStart);
        dashTrailMeshObjects[0]->SetLocalTransform(float4x4::FromTRS(float3(0, 0, -distanceFromDashStart / 2.f),
            Quat::identity, float3(1, .4f, distanceFromDashStart)));
        dashTrailColliderObjects[0]->SetLocalTransform(float4x4::FromTRS(float3(0, 0, -distanceFromDashStart / 2.f),
            Quat::identity, float3(1, 1, 1)));
        dashAreaColliders[0]->size = float3(.5f, .2f, distanceFromDashStart / 2.f);
        dashAreaColliders[0]->UpdateCollider();
    }
}

void Changeling::UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
        animComponent->UseTrigger("Trigger_VisibleIdle");
    if (stateTimer < 0.f)
    {
        for (auto dashTrailMeshObject : dashTrailMeshObjects)
            dashTrailMeshObject->SetEnabled(false);
    
        for (auto dashTrailColliderObject : dashTrailColliderObjects)
            dashTrailColliderObject->SetEnabled(false);
        //dashTrailMeshObjects[0]->SetLocalTransform(float4x4::identity);
        
        if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

        agentAI->ResetSpeed();

        if (animComponent) animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
}

void Changeling::UpdateDashChainAttackState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        if (dashIndex != 3)
        {
            const float2 xzDirectionToPlayer = (character->GetGlobalTransform().TranslatePart().xz() - parent->GetGlobalTransform().TranslatePart().xz()).Normalized();
            float3 directionToPlayer = float3(xzDirectionToPlayer.x, 0, xzDirectionToPlayer.y);
            if (dashIndex == 0)
            {
                const float dot = character->GetFrontDirection().Dot(directionToPlayer.Cross(float3(0, 1, 0)));
                dashRight = dot > 0;
            }

            const float dashAngleRads = dashAngleDegrees * DEGREE_RAD_CONV * (dashRight ? 1.f : -1.f);
            directionToPlayer = Quat::FromEulerXYZ(0, dashAngleRads, 0).Mul(directionToPlayer);

            if (!CalculateDashTargetPoint(parent->GetGlobalTransform().TranslatePart() + directionToPlayer, dashTarget))
            {
                weaponCollider->SetEnabled(false);
                isAttacking = false;
                stateTimer = attackCooldown;
                if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
                currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
            }
            dashLegacyTransforms[dashIndex] = dashTrailMeshObjects[dashIndex]->GetGlobalTransform();
            dashColliderLegacyTransforms[dashIndex] = dashTrailColliderObjects[dashIndex]->GetGlobalTransform();
            dashIndex++;
            agentAI->SetPathNavigation(dashTarget);
            agentAI->LookAtMovement(dashTarget, 1000000);
            dashTrailMeshObjects[dashIndex]->SetEnabled(true);   
            dashTrailColliderObjects[dashIndex]->SetEnabled(true);
            stateTimer = attackDuration;
        }
        else
        {
            weaponCollider->SetEnabled(false);
            isAttacking = false;
            stateTimer = attackCooldown;
            if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
            currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
        }
        
    } else {
        float distanceFromDashStart = parent->GetGlobalTransform().TranslatePart().Distance(dashStart);
        dashTrailMeshObjects[dashIndex]->SetLocalTransform(float4x4::FromTRS(float3(0, 0, -distanceFromDashStart / 2.f),
            Quat::identity, float3(1, .4f, distanceFromDashStart)));
        dashTrailColliderObjects[dashIndex]->SetLocalTransform(float4x4::FromTRS(float3(0, 0, -distanceFromDashStart / 2.f),
            Quat::identity, float3(1, 1, 1)));
        dashAreaColliders[dashIndex]->size = float3(.5f, .2f, distanceFromDashStart / 2.f);
        dashAreaColliders[dashIndex]->UpdateCollider();

        // Update transforms of previous trails meshes and collisions
        if (dashIndex != 0)
        {
            for (unsigned short i = dashIndex - 1;; --i) // End condition inside the loop
            {
                dashTrailMeshObjects[i]->SetLocalTransform(parent->GetGlobalTransform().Inverted() * dashLegacyTransforms[i]);
                dashTrailColliderObjects[i]->SetLocalTransform(parent->GetGlobalTransform().Inverted() * dashColliderLegacyTransforms[i]);
                dashAreaColliders[i]->UpdateCollider();
                
                if (i == 0) break;
            }
        }
        
    }
}

void Changeling::UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        stateTimer = biteAttackCooldown;
        characterCollider->SetEnabled(false);
        weaponCollider->SetEnabled(false);
        animComponent->UseTrigger("Trigger_BurriedIdle");
        currentState = ChangelingStates::BITE_ATTACK_COOLDOWN;
    }
}

void Changeling::UpdateBiteAttackCooldownState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f && !ST_BiteAttack(deltaTime, distanceToPlayerSq))
        currentState = ChangelingStates::IDLE_BURRIED;
}

void Changeling::UpdateDamagedState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        currentState = ChangelingStates::IDLE_VISIBLE;
        animComponent->UseTrigger("Trigger_VisibleIdle");
    }
}

void Changeling::UpdateDyingState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        isDead = true;
        parent->SetEnabled(false);
    }
}

bool Changeling::ST_Peek(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version != ChangelingVersions::HERBERT) return false;
    if (currentState != ChangelingStates::IDLE_BURRIED) return false;
    
    // Implement state transition
    characterCollider->SetEnabled(true);
    if (animComponent) animComponent->UseTrigger("Trigger_Peek");
    currentState = ChangelingStates::PEEK;

    return true;
}

bool Changeling::ST_DashAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version == ChangelingVersions::SEPP && distanceToPlayerSq > rangeAIAttack * rangeAIAttack)   return false;
    if (version == ChangelingVersions::FRANZ && distanceToPlayerSq > Pow(rangeAIAttack * 1.75f, 2))   return false;
    if (version == ChangelingVersions::HERBERT) return false;
    if (currentState != ChangelingStates::CHASE && currentState != ChangelingStates::DASH_ATTACK_COOLDOWN) return false;
    
    // Implement state transition
    
    if (!CalculateDashTargetPoint(character->GetLastPosition(), dashTarget)) return false;

    if (animComponent) animComponent->UseTrigger("Trigger_PrepareDash");
    currentState = ChangelingStates::DASH_ATTACK_PREPARATION;

    agentAI->SetLookForward(false);
    agentAI->SetSpeed(0.0f, 0.0f);
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    agentAI->LookAtMovement(dashTarget, deltaTime);

    return true;
}

bool Changeling::ST_BiteAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (distanceToPlayerSq > biteAttackRadius * biteAttackRadius)   return false;
    if (currentState != ChangelingStates::IDLE_BURRIED && currentState != ChangelingStates::BITE_ATTACK_COOLDOWN) return false;
    
    // Implement state transition
    characterCollider->SetEnabled(true);
    if (animComponent) animComponent->UseTrigger("Trigger_Bite");
    currentState = ChangelingStates::BITE_ATTACK;

    weaponCollider->SetEnabled(true);

    Character::Attack(deltaTime);

    return true;
}

void Changeling::ValidateSetup()
{
    isSetupCorrectly = true;

    // Validate variant input
    if (userSelectedVersion < 0 || userSelectedVersion > 3)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Variant input for changeling needs to be on of [0, 1, 2, 3]")
        return;
    }
    
    // Validate agentAI
    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] AIAgentComponent not found")
        return;
    }

    // Validate children game objects
    for (UID childUID : parent->GetChildren())
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child == nullptr)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Child game object is nullptr")
            return;
        }

        if (child->GetName() == dashTrailMeshName)
        {
            dashTrailMeshObjects.emplace_back(child);
        } else if (child->GetName() == dashTrailCollisionName)
        {
            dashTrailColliderObjects.emplace_back(child);
        }
    }

    if (userSelectedVersion == 0 || userSelectedVersion == 3)
    {
        // Need four dashTrailMeshObjects and four dashTrailColliderObjects
        if (dashTrailMeshObjects.size() < 4)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Four DashTrailMeshObjects are needed for version 3 of the changeling")
            return;
        }

        if (dashTrailColliderObjects.size() < 4)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Four DashTrailColliderObjects are needed for version 3 of the changeling")
            return;
        }

        dashLegacyTransforms = {float4x4(), float4x4(), float4x4()};
        dashColliderLegacyTransforms = {float4x4(), float4x4(), float4x4()};
    } else
    {
        if (dashTrailMeshObjects.empty())
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] DashTrailMeshObject not found")
            return;
        }
        if (dashTrailColliderObjects.empty())
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] DashTrailColliderObject not found")
            return;
        }
    }

    for (const auto & dashTrailColliderObject : dashTrailColliderObjects)
    {
        CubeColliderComponent* cCComponent = dashTrailColliderObject->GetComponent<CubeColliderComponent*>();
        if (cCComponent == nullptr)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] DashTrailColliderObject does not contain a cube collider")
            return;
        }

        dashAreaColliders.emplace_back(cCComponent);
    }
}

void Changeling::RenderDebugVisuals()
{
    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string versionNbr      = "Version: " + std::to_string((int)this->version);
        const std::string life      = "Health: " + std::to_string(currentHealth);
        const std::string animState = "Anim state: " + stateName.GetString();
        const std::string characterState = "Character state: " + std::to_string((int)currentState);

        std::vector<std::pair<std::string, float2>> logs {
                {versionNbr,float2(-50.0f, -120.0f)},
                {life,      float2(-50.0f, -140.0f)},
                {animState, float2(-80.0f, -160.0f)},
                {characterState, float2(-100.0f, -180.0f)},
            };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

bool Changeling::CalculateDashTargetPoint(const float3& aimingPoint, float3& targetPoint)
{
    dashStart = parent->GetGlobalTransform().TranslatePart();
    dashDirection = aimingPoint - dashStart;
    dashDirection.Normalize();
    dashDirection.y = 0;

    float3 intermediateTargetPoint;
    float3 resultPos;
    float testDistance = rangeAIAttack;
    bool posOverPoly        = false;
    const float3 searchArea = {1.0f, 1.0f, 1.0f};

    do
    {
        intermediateTargetPoint = dashStart + dashDirection * testDistance;
        agentAI->GetClosestPointInNavmesh(targetPoint, searchArea, posOverPoly, resultPos);
        if (!posOverPoly) testDistance -= 1;
    } while (!posOverPoly && testDistance > 0.0f);

    if (posOverPoly)
        targetPoint = intermediateTargetPoint;
    
    return posOverPoly;
}
