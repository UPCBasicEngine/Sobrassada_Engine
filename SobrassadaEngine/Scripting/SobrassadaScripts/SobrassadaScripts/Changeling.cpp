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

    fields.emplace_back(
        "Abs spotted reaction time", InspectorField::FieldType::Float, &absoluteSpottedReactionTime, 0.1f, 10.0f
    );

    fields.emplace_back("Bite attack radius", InspectorField::FieldType::Float, &biteAttackRadius, 0.1f, 10.0f);
    fields.emplace_back("Bite attack cooldown", InspectorField::FieldType::Float, &biteAttackCooldown, 0.1f, 10.0f);

    fields.emplace_back("Dash speed", InspectorField::FieldType::Float, &dashSpeed, 0.1f, 100.0f);
    fields.emplace_back("Min dash distance", InspectorField::FieldType::Float, &minDashDistance, 0.1f, 100.0f);

    // Version selection (0 random, 1 default, 2 sneak, 3 block)
    fields.emplace_back("Version (0: Random)", InspectorField::FieldType::Int, &userSelectedVersion, 0, 2);
    fields.emplace_back(
        "Swap states chance per second (Only with version 0)", InspectorField::FieldType::Float,
        &swapStateChancePerSecond, 0.001f, 1.0f
    );

    // Herbert specific (Index 1)
    fields.emplace_back("Chase speed", InspectorField::FieldType::Float, &chaseSpeed, 0.1f, 10.0f);
    fields.emplace_back("Chase Acceleration", InspectorField::FieldType::Float, &chaseAcceleration, 0.1f, 10.0f);

    // Sepp specific (Index 2)
    fields.emplace_back(
        "Max sneak angle degrees", InspectorField::FieldType::Float, &maxSneakAngleDegrees, 1.0f, 180.0f
    );
    fields.emplace_back("Min sneak speed", InspectorField::FieldType::Float, &minSneakSpeed, 0.001f, 10.0f);
    fields.emplace_back("Max sneak speed", InspectorField::FieldType::Float, &maxSneakSpeed, 0.1f, 10.0f);
    fields.emplace_back(
        "Distance to player for max sneak speed", InspectorField::FieldType::Float, &distanceToPlayerForMaxSneakSpeed,
        0.1f, 10.0f
    );
    fields.emplace_back("Sneak Acceleration", InspectorField::FieldType::Float, &sneakAcceleration, 0.1f, 10.0f);
    fields.emplace_back("Peek chance per second", InspectorField::FieldType::Float, &peekChancePerSecond, 0.1f, 10.0f);

    // Giacomo specific (Index 3)
    fields.emplace_back("Dash angle degrees", InspectorField::FieldType::Float, &dashAngleDegrees, 0.0f, 180.0f);
    fields.emplace_back("Time between dashes", InspectorField::FieldType::Float, &timeBetweenDashes, 0.0f, 10.0f);
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

    currentState = ChangelingStates::IDLE_BURIED;

    Character::Init();

    version = static_cast<ChangelingVersions>(userSelectedVersion);
    if (version == ChangelingVersions::RANDOM) randomVersion = ChangelingVersions::SNEAK;

    agentAI->RecreateAgent();
    agentAI->SetLookForward(true);
    speed = agentAI->GetSpeed();

    for (auto dashTrailMeshObject : dashTrailMeshObjects)
        dashTrailMeshObject->SetEnabled(false);
    for (auto dashTrailColliderObject : dashTrailColliderObjects)
        dashTrailColliderObject->SetEnabled(false);

    isAttacking   = false;
    attackCdTimer = attackCooldown;
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    characterCollider->SetEnabled(false);

    return true;
}

void Changeling::Update(float deltaTime)
{
    RenderDebugVisuals();

    if (agentAI != nullptr && isSetupCorrectly) Character::Update(deltaTime);
}

void Changeling::OnPlayerExitLocation()
{
    // TODO

    // const HashString& playerLocationTag = AppEngine->GetSceneModule()->GetScene()->GetPlayerLocation();
    // bool isPlayerInLocation = parent->HasTag(playerLocationTag);
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
    ST_Damaged();
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
    case ChangelingStates::IDLE_BURIED:
        UpdateIdleBuriedState(deltaTime, distanceToPlayerSq);
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
    case ChangelingStates::BURIED_CHASE:
        UpdateBuriedChaseState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DASH_ATTACK_PREPARATION:
        UpdateDashAttackPreparationState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DASH_ATTACK:
        UpdateDashAttackState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DASH_ATTACK_WIGGLE:
        UpdateDashAttackWiggleState(deltaTime, distanceToPlayerSq);
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
        currentState = ChangelingStates::IDLE_BURIED;
        break;
    }

    stateTimer -= deltaTime;
}

void Changeling::UpdateIdleBuriedState(float deltaTime, float distanceToPlayerSq)
{
    if (ShouldSwapStatesOnRandomVersion(deltaTime)) randomVersion = static_cast<ChangelingVersions>(rand() % 3 + 1);

    if (ST_BiteAttack(deltaTime, distanceToPlayerSq)) return;

    if (ST_BuryUp(deltaTime, distanceToPlayerSq)) return;
    if (ST_StartBuriedChase(deltaTime, distanceToPlayerSq)) return;

    if (ST_Peek(deltaTime, distanceToPlayerSq)) return;
}

void Changeling::UpdatePeekState(float deltaTime, float distanceToPlayerSq)
{
    if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent && animComponent->IsFinished())
    {
        if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
        {
            spottedLocation         = character->GetLastPosition();
            spottedViewingDirection = character->GetFrontDirection();
        }
        else
        {
            spottedLocation         = float3::nan;
            spottedViewingDirection = float3::nan;
        }
        characterCollider->SetEnabled(false);
        animComponent->UseTrigger("Trigger_BurriedIdle");
        currentState = ChangelingStates::IDLE_BURIED;
        stateTimer   = absoluteSpottedReactionTime; // Wait reaction time for bury up
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
        if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
        {
            spottedLocation         = character->GetLastPosition();
            spottedViewingDirection = character->GetFrontDirection();
        }
        else
        {
            spottedLocation         = float3::nan;
            spottedViewingDirection = float3::nan;
        }
        animComponent->UseTrigger("Trigger_BurriedIdle");
        currentState = ChangelingStates::IDLE_BURIED;
    }
}

void Changeling::UpdateIdleVisibleState(float deltaTime, float distanceToPlayerSq)
{
    if (ShouldSwapStatesOnRandomVersion(deltaTime))
    {
        randomVersion = static_cast<ChangelingVersions>((rand() % 3) + 1);

        GLOG("[INFO] Swapping to random version: %d", randomVersion)

        if (randomVersion == ChangelingVersions::SNEAK)
        {
            agentAI->SetSpeed(0.0f, 10.0f);
            if (animComponent) animComponent->UseTrigger("Trigger_BuryDown");
            currentState = ChangelingStates::DIG_DOWN_TRANSITION;
        }
    }

    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

    if (ST_StartChase(deltaTime, distanceToPlayerSq)) return;

    if (animComponent) animComponent->UseTrigger("Trigger_BuryDown");
    currentState = ChangelingStates::DIG_DOWN_TRANSITION;
}

void Changeling::UpdateChaseState(float deltaTime, float distanceToPlayerSq)
{
    if (ShouldSwapStatesOnRandomVersion(deltaTime))
    {
        randomVersion = static_cast<ChangelingVersions>((rand() % 3) + 1);

        GLOG("[INFO] Swapping to random version: %d", randomVersion)

        if (randomVersion == ChangelingVersions::SNEAK)
        {
            agentAI->SetSpeed(0.0f, 10.0f);
            if (animComponent) animComponent->UseTrigger("Trigger_BuryDown");
            currentState = ChangelingStates::DIG_DOWN_TRANSITION;
        }
    }

    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

    if (distanceToPlayerSq > rangeAIChase * rangeAIChase)
    {
        agentAI->SetSpeed(0.0f, 10.0f);
        if (animComponent) animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        agentAI->SetPathNavigation(character->GetLastPosition());
    }
}

void Changeling::UpdateBuriedChaseState(float deltaTime, float distanceToPlayerSq)
{
    if (ShouldSwapStatesOnRandomVersion(deltaTime))
    {
        randomVersion = static_cast<ChangelingVersions>((rand() % 3) + 1);

        GLOG("[INFO] Swapping to random version: %d", randomVersion)

        if (randomVersion != ChangelingVersions::SNEAK)
        {
            agentAI->SetSpeed(0.0f, 10.0f);
            currentState = ChangelingStates::IDLE_BURIED;
        }
    }

    agentAI->LookAtMovement(spottedLocation, deltaTime);

    if (ST_BiteAttack(deltaTime, distanceToPlayerSq)) return;

    if (ST_Peek(deltaTime, distanceToPlayerSq)) return;

    const float3 directionToPlayer  = (spottedLocation - parent->GetGlobalTransform().TranslatePart()).Normalized();
    const float angleToPlayerVision = spottedViewingDirection.AngleBetween(directionToPlayer) * RAD_DEGREE_CONV;
    if (angleToPlayerVision < maxSneakAngleDegrees)
    {
        const float lerpFactor =
            max(min((distanceToPlayerSq - Pow(distanceToPlayerForMaxSneakSpeed, 2)) /
                        Pow(maxDetectionRange - distanceToPlayerForMaxSneakSpeed, 2),
                    1),
                0);

        const float currentSneakSpeed = minSneakSpeed + (maxSneakSpeed - minSneakSpeed) * (1 - lerpFactor);

        agentAI->SetSpeed(currentSneakSpeed, sneakAcceleration);
        agentAI->SetPathNavigation(spottedLocation);
    }
    else
    {
        agentAI->SetSpeed(0, 10);
    }

    if (spottedLocation.Distance(parent->GetGlobalTransform().TranslatePart()) < 0.5f)
    {
        agentAI->SetSpeed(0.0f, 10.0f);
        currentState = ChangelingStates::IDLE_BURIED;
    }
}

void Changeling::UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq)
{
    if (version == ChangelingVersions::BLOCK || randomVersion == ChangelingVersions::BLOCK)
    {
        float3 calculatedAimPoint;
        CalculateAimPoint(calculatedAimPoint);
        agentAI->LookAtMovement(calculatedAimPoint, deltaTime);
    }
    else agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent && animComponent->IsFinished())
    {
        Character::Attack(deltaTime);

        weaponCollider->SetEnabled(true);
        dashTrailMeshObjects[0]->SetEnabled(true);
        dashTrailColliderObjects[0]->SetEnabled(true);
        dashIndex = 0;
        if (ST_AimNextDashChainAttack(deltaTime, distanceToPlayerSq))
        {
            animComponent->UseTrigger("Trigger_Dash");
            agentAI->SetSpeed(dashSpeed, 1000000);
            agentAI->SetPathNavigation(dashTarget);
            currentState = ChangelingStates::DASH_CHAIN_ATTACK;
        }
        else
        {
            ST_AimNextDashAttack(deltaTime, distanceToPlayerSq);
            animComponent->UseTrigger("Trigger_Dash");
            currentState = ChangelingStates::DASH_ATTACK;
        }
    }
}

void Changeling::UpdateDashAttackState(float deltaTime, float distanceToPlayerSq)
{
    const float distanceFromDashStart =
        parent->GetGlobalTransform().TranslatePart().Distance(dashStart.TranslatePart());
    if (activeDashRange < distanceFromDashStart)
    {
        weaponCollider->SetEnabled(false);
        isAttacking = false;
        stateTimer  = attackCooldown;
        dashIndex   = 0;
        if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
        currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
    }
    else
    {
        const float3 lerpTranslation = (dashStart.TranslatePart() + dashDirection * (distanceFromDashStart / 2.f)) -
                                       parentGO->GetGlobalTransform().TranslatePart();
        const Quat lerpRotation = Quat(dashStart.RotatePart());

        dashTrailMeshObjects[dashIndex]->SetLocalTransform(
            float4x4::FromTRS(lerpTranslation, lerpRotation, float3(1, .4f, distanceFromDashStart))
        );
        dashTrailColliderObjects[dashIndex]->SetLocalTransform(
            float4x4::FromTRS(lerpTranslation, lerpRotation, float3(1, 1, 1))
        );
        dashAreaColliders[dashIndex]->size = float3(.5f, .2f, distanceFromDashStart / 2.f);
        dashAreaColliders[dashIndex]->UpdateCollider();
    }
}

void Changeling::UpdateDashAttackWiggleState(float deltaTime, float distanceToPlayerSq)
{
    agentAI->LookAtMovement(dashTarget, deltaTime);
    if (stateTimer < 0)
    {
        dashStart.SetRotatePart(parent->GetGlobalTransform().RotatePart());

        agentAI->ResetSpeed();
        agentAI->SetSpeed(dashSpeed, 1000000);
        agentAI->SetPathNavigation(dashTarget);
        dashTrailMeshObjects[dashIndex]->SetEnabled(true);
        dashTrailColliderObjects[dashIndex]->SetEnabled(true);

        animComponent->UseTrigger("Trigger_Dash");
        currentState = bNextDashUninterrupted ? ChangelingStates::DASH_CHAIN_ATTACK : ChangelingStates::DASH_ATTACK;
    }
}

void Changeling::UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent && animComponent->IsFinished())
    {
        const bool bUseAnimation1 = rand() % 2;
        animComponent->UseTrigger(bUseAnimation1 ? "Trigger_Scream" : "Trigger_Scream2");
    }
    if (stateTimer < 0.f)
    {
        for (auto dashTrailMeshObject : dashTrailMeshObjects)
            dashTrailMeshObject->SetEnabled(false);

        for (auto dashTrailColliderObject : dashTrailColliderObjects)
            dashTrailColliderObject->SetEnabled(false);

        if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

        agentAI->ResetSpeed();

        if (animComponent) animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
}

void Changeling::UpdateDashChainAttackState(float deltaTime, float distanceToPlayerSq)
{
    const float distanceFromDashStart =
        parent->GetGlobalTransform().TranslatePart().Distance(dashStart.TranslatePart());

    if (activeDashRange < distanceFromDashStart)
    {
        if (dashIndex != 3)
        {
            ST_AimNextDashChainAttack(deltaTime, distanceToPlayerSq);

            dashIndex++;

            agentAI->SetSpeed(0, 10);
            stateTimer   = timeBetweenDashes;
            currentState = ChangelingStates::DASH_ATTACK_WIGGLE;
            if (animComponent) animComponent->UseTrigger("Trigger_Wiggle");
        }
        else
        {
            weaponCollider->SetEnabled(false);
            isAttacking = false;
            stateTimer  = attackCooldown;
            dashIndex   = 0;
            if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
            currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
        }
    }
    else
    {
        const float3 lerpTranslation = (dashStart.TranslatePart() + dashDirection * (distanceFromDashStart / 2.f)) -
                                       parentGO->GetGlobalTransform().TranslatePart();
        const Quat lerpRotation = Quat(dashStart.RotatePart());

        dashTrailMeshObjects[dashIndex]->SetLocalTransform(
            float4x4::FromTRS(lerpTranslation, lerpRotation, float3(1, .4f, distanceFromDashStart))
        );
        dashTrailColliderObjects[dashIndex]->SetLocalTransform(
            float4x4::FromTRS(lerpTranslation, lerpRotation, float3(1, 1, 1))
        );
        dashAreaColliders[dashIndex]->size = float3(.5f, .2f, distanceFromDashStart / 2.f);
        dashAreaColliders[dashIndex]->UpdateCollider();
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
    if (stateTimer < 0.f && !ST_BiteAttack(deltaTime, distanceToPlayerSq)) currentState = ChangelingStates::IDLE_BURIED;
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

bool Changeling::ST_BuryUp(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version == ChangelingVersions::SNEAK || randomVersion == ChangelingVersions::SNEAK) return false;
    if (currentState != ChangelingStates::IDLE_BURIED) return false;
    if (!spottedLocation.IsFinite()) return false;

    // Implement state transition
    if (stateTimer <= 0.f)
    {
        characterCollider->SetEnabled(true);
        if (animComponent) animComponent->UseTrigger("Trigger_BuryUp");
        currentState = ChangelingStates::DIG_UP_TRANSITION;
    }

    return true;
}

bool Changeling::ST_StartChase(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (currentState != ChangelingStates::IDLE_VISIBLE) return false;
    if (distanceToPlayerSq > rangeAIChase * rangeAIChase) return false;

    // Implement state transition
    const bool bUseAnimation1 = rand() % 2;
    if (animComponent) animComponent->UseTrigger(bUseAnimation1 ? "Trigger_Run" : "Trigger_Run2");

    agentAI->ResetSpeed();
    agentAI->SetSpeed(chaseSpeed, chaseAcceleration);

    currentState = ChangelingStates::CHASE;

    return true;
}

bool Changeling::ST_StartBuriedChase(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (currentState != ChangelingStates::IDLE_BURIED) return false;
    if (version != ChangelingVersions::SNEAK && randomVersion != ChangelingVersions::SNEAK) return false;
    if (!spottedLocation.IsFinite()) return false;

    // Implement state transition
    agentAI->ResetSpeed();
    currentState = ChangelingStates::BURIED_CHASE;

    return true;
}

bool Changeling::ST_Damaged()
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
        const bool bUseAnimation1 = rand() % 2;
        if (!animComponent->UseTrigger(bUseAnimation1 ? "Trigger_Hit" : "Trigger_Hit2"))
            animComponent->UseTrigger("Trigger_HitUnderground");
    }

    return true;
}

bool Changeling::ST_Peek(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (currentState != ChangelingStates::IDLE_BURIED && currentState != ChangelingStates::BURIED_CHASE) return false;

    // Implement state transition
    const int randomValue = max(1, (int)round(1.0f / (peekChancePerSecond * deltaTime)));

    // Random value = 1 only if fps are too high -> Ignore those frames then
    if (randomValue == 1 || rand() % randomValue != 0) return false; // Only peek randomly

    characterCollider->SetEnabled(true);
    agentAI->SetSpeed(0.0f, 10.0f);
    if (animComponent) animComponent->UseTrigger("Trigger_Peek");
    currentState = ChangelingStates::PEEK;

    return true;
}

bool Changeling::ST_DashAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version != ChangelingVersions::SNEAK && distanceToPlayerSq > rangeAIAttack * rangeAIAttack) return false;
    if (version == ChangelingVersions::SNEAK || randomVersion == ChangelingVersions::SNEAK) return false;
    if (currentState != ChangelingStates::CHASE && currentState != ChangelingStates::DASH_ATTACK_COOLDOWN) return false;

    // Reset parameters
    dashStart = float4x4();
    dashStart.SetTranslatePart(float3::inf);
    dashStart.SetRotatePart(Quat::nan);

    // Implement state transition

    if (animComponent) animComponent->UseTrigger("Trigger_PrepareDash");
    currentState = ChangelingStates::DASH_ATTACK_PREPARATION;

    agentAI->SetLookForward(false);
    agentAI->SetSpeed(0.0f, 10.0f);
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    return true;
}

bool Changeling::ST_AimNextDashChainAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (version != ChangelingVersions::BLOCK && randomVersion != ChangelingVersions::BLOCK) return false;

    float3 calculatedAimPoint;

    CalculateAimPoint(calculatedAimPoint);

    bNextDashUninterrupted = CalculateDashTargetPoint(calculatedAimPoint, dashTarget);

    if (minDashDistance > activeDashRange)
    {
        weaponCollider->SetEnabled(false);
        isAttacking = false;
        stateTimer  = attackCooldown;
        if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
        currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
    }

    return bNextDashUninterrupted;
}

bool Changeling::ST_AimNextDashAttack(float deltaTime, float distanceToPlayerSq)
{
    CalculateDashTargetPoint(character->GetLastPosition(), dashTarget);

    if (minDashDistance > activeDashRange)
    {
        isAttacking = false;
        stateTimer  = attackCooldown;
        if (animComponent) animComponent->UseTrigger("Trigger_FinishDash");
        currentState = ChangelingStates::DASH_ATTACK_COOLDOWN;
        return false;
    }

    agentAI->SetSpeed(dashSpeed, 1000000);
    agentAI->SetPathNavigation(dashTarget);

    return true;
}

bool Changeling::ST_BiteAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (distanceToPlayerSq > biteAttackRadius * biteAttackRadius) return false;
    if (currentState != ChangelingStates::IDLE_BURIED && currentState != ChangelingStates::BURIED_CHASE &&
        currentState != ChangelingStates::BITE_ATTACK_COOLDOWN)
        return false;

    // Implement state transition
    characterCollider->SetEnabled(true);
    agentAI->SetSpeed(0.0f, 10.0f);
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

    parentGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetParent());
    if (parentGO == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Parent game object not found")
        return;
    }

    // Validate children game objects
    for (const UID childUID : parentGO->GetChildren())
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
        }
        else if (child->GetName() == dashTrailCollisionName)
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
    }
    else
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

    for (const auto& dashTrailColliderObject : dashTrailColliderObjects)
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
        const std::string versionNbr     = "Version: " + std::to_string((int)this->version);
        const std::string life           = "Health: " + std::to_string(currentHealth);
        const std::string animState      = "Anim state: " + stateName.GetString();
        const std::string characterState = "Character state: " + std::to_string((int)currentState);
        const std::string setupState     = isSetupCorrectly ? "Setup valid" : "Setup invalid";
        const std::string behavesLikeHerbert =
            version == ChangelingVersions::RANDOM ? std::to_string((int)this->randomVersion) : "Wrong version";

        std::vector<std::pair<std::string, float2>> logs {
            {versionNbr,         float2(-50.0f,  -120.0f)},
            {life,               float2(-50.0f,  -140.0f)},
            {animState,          float2(-80.0f,  -160.0f)},
            {characterState,     float2(-100.0f, -180.0f)},
            {behavesLikeHerbert, float2(-100.0f, -200.0f)},
            {setupState,         float2(-80.0f,  -220.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.0f, 0.0f));
    }
}

bool Changeling::CalculateDashTargetPoint(const float3& aimingPoint, float3& targetPoint)
{
    dashStart     = parent->GetGlobalTransform();
    dashDirection = aimingPoint - dashStart.TranslatePart();
    dashDirection.Normalize();
    dashDirection.y = 0;

    float3 intermediateTargetPoint;
    float3 resultPos;
    float testDistance      = rangeAIAttack;
    bool posOverPoly        = false;
    bool uninterruptedDash  = true;
    const float3 searchArea = {1.0f, 1.0f, 1.0f};

    do
    {
        intermediateTargetPoint = dashStart.TranslatePart() + dashDirection * testDistance;
        agentAI->GetClosestPointInNavmesh(intermediateTargetPoint, searchArea, posOverPoly, resultPos);
        if (!posOverPoly)
        {
            uninterruptedDash  = false;
            testDistance      -= 1;
        }
    } while (!posOverPoly && testDistance > 0.0f);

    if (posOverPoly) targetPoint = intermediateTargetPoint;

    // -0.5f: Reduce target distance so the dash always stops
    activeDashRange = targetPoint.Distance(dashStart.TranslatePart()) - 0.5f;

    return uninterruptedDash;
}

bool Changeling::ShouldSwapStatesOnRandomVersion(const float deltaTime) const
{
    // Check preconditions
    if (version != ChangelingVersions::RANDOM) return false;

    // Implement state transition
    const int swapValue = max(1, (int)round(1.0f / (swapStateChancePerSecond * deltaTime)));

    // Random value = 1 only if fps are too high -> Ignore those frames then
    if (swapValue == 1 || rand() % swapValue != 0) return false;

    return true;
}

void Changeling::CalculateAimPoint(float3& outTargetPoint)
{
    float3 directionToAimPoint;
    if (dashIndex == 0)
    {
        const float2 xzDirectionToPlayer =
            (character->GetGlobalTransform().TranslatePart().xz() - parent->GetGlobalTransform().TranslatePart().xz())
                .Normalized();
        float3 directionToPlayer  = float3(xzDirectionToPlayer.x, 0, xzDirectionToPlayer.y);
        const float dot           = character->GetFrontDirection().Dot(directionToPlayer.Cross(float3(0, 1, 0)));
        dashRight                 = dot > 0;
        const float dashAngleRads = dashAngleDegrees * DEGREE_RAD_CONV * (dashRight ? -1.f : 1.f);
        directionToAimPoint       = Quat::FromEulerXYZ(0, dashAngleRads, 0).Mul(directionToPlayer);
    }
    else
    {
        const float dashAngleRads = dashAngleDegrees * DEGREE_RAD_CONV * (dashRight ? 1.f : -1.f);
        directionToAimPoint       = Quat::FromEulerXYZ(0, dashAngleRads, 0).Mul(dashDirection);
    }

    outTargetPoint = parent->GetGlobalTransform().TranslatePart() + directionToAimPoint;
}
