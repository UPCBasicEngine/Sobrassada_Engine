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
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

#include <Math/MathFunc.h>
#include <Math/Quat.h>

Changeling::Changeling(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Changeling)
{
    fields.emplace_back("Dash trail mesh", InspectorField::FieldType::InputText, &dashTrailMeshName);
    fields.emplace_back("Dash trail collision", InspectorField::FieldType::InputText, &dashTrailCollisionName);
    fields.emplace_back("Body mesh", InspectorField::FieldType::InputText, &bodyMeshPath);
    
    fields.emplace_back("Abs spotted reaction time", InspectorField::FieldType::Float, &absoluteSpottedReactionTime, 0.1f, 10.0f);
    fields.emplace_back("Abs rise duration", InspectorField::FieldType::Float, &absoluteRiseDuration, 0.1f, 10.0f);

    fields.emplace_back("Dash attack preparation duration", InspectorField::FieldType::Float, &dashAttackPreparationDuration, 0.1f, 10.0f);
    fields.emplace_back("Bite attack radius", InspectorField::FieldType::Float, &biteAttackRadius, 0.1f, 10.0f);
    fields.emplace_back("Bite attack duration", InspectorField::FieldType::Float, &biteAttackDuration, 0.1f, 10.0f);
    fields.emplace_back("Bite attack cooldown", InspectorField::FieldType::Float, &biteAttackCooldown, 0.1f, 10.0f);
    
    fields.emplace_back("Dying duration", InspectorField::FieldType::Float, &dyingDuration, 0.1f, 10.0f);

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
    fields.emplace_back("Peek duration", InspectorField::FieldType::Float, &peekDuration, 0.001f, 1.0f);

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

    currentState = ChangelingStates::HIDDEN;

    Character::Init();

    version = static_cast<ChangelingVersions>(userSelectedVersion == 0 ? rand() % 3 + 1 : userSelectedVersion);

    agentAI->RecreateAgent();
    agentAI->SetLookForward(true);
    speed = agentAI->GetSpeed();

    for (auto dashTrailMeshObject : dashTrailMeshObjects)
        dashTrailMeshObject->SetEnabled(false);
    for (auto dashTrailColliderObject : dashTrailColliderObjects)
        dashTrailColliderObject->SetEnabled(false);
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
    // Don´t update a changeling with wrong setup
    if (!isSetupCorrectly) return;
    
    float distanceToPlayerSq = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());

    switch (currentState)
    {
    case ChangelingStates::HIDDEN:
        UpdateHiddenState(deltaTime, distanceToPlayerSq);
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
    
}

void Changeling::UpdatePeekState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
        bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));
        characterCollider->SetEnabled(false);
        currentState = ChangelingStates::HIDDEN;
    }
}

void Changeling::UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
    {
        if (stateTimer < 0.f)
        {
            bodyMeshObject->SetLocalPosition(float3(0, 0, 0));
            currentState = ChangelingStates::CHASE;
        } else
        {
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
            bodyMeshObject->SetLocalPosition(float3(0, -1.2f, 0));
            characterCollider->SetEnabled(false);
            currentState = ChangelingStates::HIDDEN;
        } else
        {
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
        agentAI->SetSpeed(0.0f, 10.0f);
        currentState = ChangelingStates::DIG_DOWN_TRANSITION;
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
    
    if (stateTimer < 0.f)
    {
        Character::Attack(deltaTime);
    
        agentAI->SetSpeed(dashSpeed, 1000000);
        agentAI->SetPathNavigation(dashTarget);
        stateTimer = attackDuration;
        
        weaponCollider->SetEnabled(true);
        dashTrailMeshObjects[0]->SetEnabled(true);   
        dashTrailColliderObjects[0]->SetEnabled(true);
        if (version == ChangelingVersions::GIACOMO)
        {
            dashIndex = 0;
            currentState = ChangelingStates::DASH_CHAIN_ATTACK;
        } else
        {
            currentState = ChangelingStates::DASH_ATTACK;
        }
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
    if (stateTimer < 0.f)
    {
        for (auto dashTrailMeshObject : dashTrailMeshObjects)
            dashTrailMeshObject->SetEnabled(false);
    
        for (auto dashTrailColliderObject : dashTrailColliderObjects)
            dashTrailColliderObject->SetEnabled(false);
        //dashTrailMeshObjects[0]->SetLocalTransform(float4x4::identity);
        
        if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;
        
        stateTimer = absoluteRiseDuration;
        agentAI->ResetSpeed();
        currentState = ChangelingStates::DIG_DOWN_TRANSITION;
    }
}

void Changeling::UpdateDashChainAttackState(float deltaTime, float distanceToPlayerSq)
{
    if (dashIndex == 2) return;
    if (stateTimer < 0.f)
    {
        if (dashIndex == 1) return;
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
                currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
            }
            dashLegacyTransforms[dashIndex] = parent->GetGlobalTransform();
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
                GLOG("PG: %f, %f, %f ; PL: %f, %f, %f", parent->GetParentGlobalTransform().TranslatePart().x, parent->GetParentGlobalTransform().TranslatePart().y, parent->GetParentGlobalTransform().TranslatePart().z,
                    dashLegacyTransforms[i].TranslatePart().x, dashLegacyTransforms[i].TranslatePart().y, dashLegacyTransforms[i].TranslatePart().z)
                float3 localPosition = parent->GetParentGlobalTransform().TranslatePart() - dashLegacyTransforms[i].TranslatePart();
                
                Quat localRotation = parent->GetParentGlobalTransform().RotatePart().ToQuat().Mul(dashLegacyTransforms[i].RotatePart().ToQuat().Inverted());

                dashTrailMeshObjects[i]->SetLocalPosition(localPosition);
                //dashTrailMeshObjects[i]->SetLocalTransform(float4x4::FromTRS(localPosition, localRotation, dashTrailMeshObjects[i]->GetLocalTransform().GetScale()));
                //dashTrailColliderObjects[i]->SetLocalPosition(dashTrailColliderObjects[i]->GetLocalTransform().operator+(parent->GetGlobalTransform().operator-(dashLegacyTransforms[i])));
                //dashLegacyTransforms[i] = parent->GetGlobalTransform();
                if (i == 0) break;
            }
        }
        
    }
}

void Changeling::UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 0.f)
    {
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
        bodyMeshObject->SetLocalPosition(Lerp(float3(0, -2.f,0 ), float3(0, 0, 0), stateTimer / dyingDuration));
    }
}

bool Changeling::ST_Peek(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version != ChangelingVersions::HERBERT) return false;
    if (currentState != ChangelingStates::HIDDEN) return false;
    
    // Implement state transition
    bodyMeshObject->SetLocalPosition(float3(0, -.6f, 0));
    characterCollider->SetEnabled(true);
    stateTimer = peekDuration;
    currentState = ChangelingStates::PEEK;

    return true;
}

bool Changeling::ST_DashAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version == ChangelingVersions::SEPP && distanceToPlayerSq > rangeAIAttack * rangeAIAttack)   return false;
    if (version == ChangelingVersions::GIACOMO && distanceToPlayerSq > Pow(rangeAIAttack * 1.5f, 2))   return false;
    if (version == ChangelingVersions::HERBERT) return false;
    if (currentState != ChangelingStates::CHASE && currentState != ChangelingStates::DASH_ATTACK_COOLDOWN) return false;
    
    // Implement state transition
    
    if (!CalculateDashTargetPoint(character->GetLastPosition(), dashTarget)) return false;
    
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
    bodyMeshObject->SetLocalPosition(float3(0, -.8f, 0));
    characterCollider->SetEnabled(true);
    hasPlayerSpotted = true;
    stateTimer = biteAttackDuration;
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
        } else if (child->GetName() == bodyMeshPath)
        {
            bodyMeshObject = child;
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
    
    if (bodyMeshObject == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] BodyMeshObject not found")
        return;
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

        std::vector<std::pair<std::string, float2>> logs {
                {versionNbr,float2(-50.0f, -120.0f)},
                {life,      float2(-50.0f, -140.0f)},
                {animState, float2(-80.0f, -160.0f)},
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
