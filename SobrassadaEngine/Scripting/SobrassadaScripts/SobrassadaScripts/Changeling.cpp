#include "pch.h"

#include "Application.h"
#include "Changeling.h"

#include "AttackVfxSpritesheet.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "Globals.h"
#include "MagicBarrier.h"
#include "Projectile.h"
#include "ResourceStateMachine.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimController.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Wwise_IDs.h"

#include <Math/MathFunc.h>
#include <Math/Quat.h>

Changeling::Changeling(GameObject* parent)
    : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Changeling)
{
    fields.emplace_back("Dash trail object", InspectorField::FieldType::InputText, &dashTrailObjectName);
    fields.emplace_back("Dash trail start mesh", InspectorField::FieldType::InputText, &dashTrailStartMeshName);
    fields.emplace_back("Dash trail mid base", InspectorField::FieldType::InputText, &dashTrailMidBaseName);
    fields.emplace_back("Dash trail mid mesh", InspectorField::FieldType::InputText, &dashTrailMidMeshName);
    fields.emplace_back("Dash trail end mesh", InspectorField::FieldType::InputText, &dashTrailEndMeshName);
    fields.emplace_back("Dash trail collision", InspectorField::FieldType::InputText, &dashTrailCollisionName);
    fields.emplace_back("Final attack collision", InspectorField::FieldType::InputText, &finalAttackColliderName);

    fields.emplace_back(
        "Abs spotted reaction time", InspectorField::FieldType::Float, &absoluteSpottedReactionTime, 0.1f, 10.0f
    );

    fields.emplace_back("Bite attack radius", InspectorField::FieldType::Float, &biteAttackRadius, 0.1f, 10.0f);
    fields.emplace_back("Bite attack cooldown", InspectorField::FieldType::Float, &biteAttackCooldown, 0.1f, 10.0f);

    fields.emplace_back("Dash speed", InspectorField::FieldType::Float, &dashSpeed, 0.1f, 100.0f);
    fields.emplace_back("Min dash distance", InspectorField::FieldType::Float, &minDashDistance, 0.1f, 100.0f);
    
    fields.emplace_back(
        "Max enemies left for final attack", InspectorField::FieldType::Int, &maxEnemiesLeftForFinalAttack, 0, 40
    );
    fields.emplace_back("Peek chance per second", InspectorField::FieldType::Float, &peekChancePerSecond, 0.1f, 10.0f);
    fields.emplace_back("Buried travel speed", InspectorField::FieldType::Float, &buriedTravelSpeed, 0.1f, 10.0f);

    fields.emplace_back("Chase speed", InspectorField::FieldType::Float, &chaseSpeed, 0.1f, 10.0f);
    fields.emplace_back("Chase Acceleration", InspectorField::FieldType::Float, &chaseAcceleration, 0.1f, 10.0f);

    // VFX
    fields.emplace_back("Dig up rocks name", InspectorField::FieldType::InputText, &vfxDigUpRocksName);
    fields.emplace_back("Dig up hole name", InspectorField::FieldType::InputText, &vfxDigUpHoleName);
    fields.emplace_back("Dash trail vfx name", InspectorField::FieldType::InputText, &vfxDashTrailName);
    fields.emplace_back("Drop down vfx name", InspectorField::FieldType::InputText, &vfxDropDownName);
    fields.emplace_back("Dash vfx name", InspectorField::FieldType::InputText, &vfxDashName);
    fields.emplace_back("Dig down vfx name", InspectorField::FieldType::InputText, &vfxDigDownName);
    fields.emplace_back("Bite vfx name", InspectorField::FieldType::InputText, &vfxBiteName);

    // Highlight
    fields.emplace_back("Highlight duration", InspectorField::FieldType::Float, &highlightDuration, 0.1f, 10.0f);
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

    agentAI->RecreateAgent();
    agentAI->SetLookForward(true);
    speed = agentAI->GetSpeed();

    dashTrailMeshObject.dashTrailObject->SetEnabled(false);
    dashTrailColliderObject->SetEnabled(false);

    vfxDropDown->SetEnabled(false);
    vfxDash->SetEnabled(false);
    vfxDigDown->SetEnabled(false);
    vfxBite->SetEnabled(false);
    vfxDig->SetEnabled(false);

    isAttacking   = false;
    attackCdTimer = attackCooldown;
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    characterCollider->SetEnabled(false);

    vfxDigUpRocksObject->SetEnabled(false);
    vfxDigUpHoleObject->SetEnabled(false);

    return true;
}

void Changeling::Update(float deltaTime)
{
    RenderDebugVisuals();

    if (agentAI != nullptr && isSetupCorrectly) Character::Update(deltaTime);
}

void Changeling::OnPlayerExitLocation()
{
    // If updated in the visible range, the pooka will automatically burry and return to idle when the player
    // is no longer visible
}

void Changeling::OnPlayerEnterLocation()
{
}

void Changeling::PlayHighlightSequence()
{
    // Don´t play the highlight if the pooka is already doing something else
    if (currentState == ChangelingStates::IDLE_BURIED || currentState == ChangelingStates::PEEK)
    {
        currentState             = ChangelingStates::HIGHLIGHTING;
        currentHighlightingState = HighlightingStates::IDLE;
    }
}

void Changeling::OnDeath()
{
    isDead = false; // TODO To keep getting updates until the death animation is finished
    dashTrailMeshObject.dashTrailObject->SetEnabled(false);

    CubeColliderComponent* collider = dashTrailColliderObject->GetComponent<CubeColliderComponent*>();
    if (collider != nullptr) collider->DeleteRigidBody();
    dashTrailColliderObject->SetEnabled(false);

    animComponent->UseTrigger("Trigger_VisibleIdle");
    animComponent->UseTrigger("Trigger_Die");
    audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_DEATH);
    agentAI->SetSpeed(0, 10);
    currentState = ChangelingStates::DYING;
}

void Changeling::OnDamageTaken(int amount)
{
    ST_Damaged();
}

void Changeling::PerformAttack()
{
    // TODO: deactivate the collision box to avoid multi damage?
}

void Changeling::HandleState(float deltaTime)
{
    // Don´t update a changeling with wrong setup
    if (!isSetupCorrectly) return;

    float distanceToPlayerSq = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());
    bool lastAttack =
        associatedBarrier != nullptr && associatedBarrier->GetEnemiesInArea() <= maxEnemiesLeftForFinalAttack;

    switch (currentState)
    {
    case ChangelingStates::IDLE_BURIED:
        UpdateIdleBuriedState(deltaTime, distanceToPlayerSq, lastAttack);
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
    case ChangelingStates::BURIED_TRAVEL:
        UpdateBuriedTravelState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::IDLE_VISIBLE:
        UpdateIdleVisibleState(deltaTime, distanceToPlayerSq, lastAttack);
        break;
    case ChangelingStates::CHASE:
        UpdateChaseState(deltaTime, distanceToPlayerSq, lastAttack);
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
    case ChangelingStates::DAMAGED:
        UpdateDamagedState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::DYING:
        UpdateDyingState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::HIGHLIGHTING:
        UpdateHighlightState(deltaTime, distanceToPlayerSq);
        break;
    case ChangelingStates::NONE:
        currentState = ChangelingStates::IDLE_BURIED;
        break;
    }

    stateTimer -= deltaTime;
}

void Changeling::UpdateIdleBuriedState(float deltaTime, float distanceToPlayerSq, bool lastAttack)
{
    if (ST_BiteAttack(deltaTime, distanceToPlayerSq)) return;

    if (ST_BuryUp(deltaTime, distanceToPlayerSq, lastAttack)) return;

    if (ST_Peek(deltaTime, distanceToPlayerSq)) return;
}

void Changeling::UpdatePeekState(float deltaTime, float distanceToPlayerSq)
{
    if (distanceToPlayerSq <= rangeAIChase * rangeAIChase)
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent->IsFinished())
    {
        spottedLocation =
            distanceToPlayerSq <= maxDetectionRange * maxDetectionRange ? character->GetLastPosition() : float3::nan;
        characterCollider->SetEnabled(false);
        animComponent->UseTrigger("Trigger_BurriedIdle");
        currentState = ChangelingStates::IDLE_BURIED;
        stateTimer   = absoluteSpottedReactionTime; // Wait reaction time for bury up
    }
}

void Changeling::UpdateDigUpTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent->IsFinished())
    {
        vfxDigUpRocksObject->SetEnabled(false);
        vfxDigUpHoleObject->SetEnabled(false);
        animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
}

void Changeling::UpdateDigDownTransitionState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer < 1.3f && stateTimer > 0.8f && !vfxDig->GetEnabled())
    {
        const float3 dir    = parent->GetGlobalTransform().WorldZ();
        const float3 offset = float3(dir.x * 0.8f, 0.25f, dir.z * 0.8f);

        vfxDig->GetParent()->SetLocalPosition(
            parent->GetGlobalTransform().TranslatePart() + offset - parent->GetParentGlobalTransform().TranslatePart()
        );
        vfxDig->SetEnabled(true);
        vfxDig->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }
    if (stateTimer <= 0)
    {
        characterCollider->SetEnabled(false);
        if (distanceToPlayerSq <= maxDetectionRange * maxDetectionRange)
        {
            spottedLocation = character->GetLastPosition();
            stateTimer      = SqrtFast(distanceToPlayerSq) / buriedTravelSpeed;
            animComponent->GetAnimationController()->Pause();
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_UNDERGROUND);
            currentState = ChangelingStates::BURIED_TRAVEL;
        }
        else if (animComponent->IsFinished())
        {
            spottedLocation = float3::nan;
            animComponent->UseTrigger("Trigger_BurriedIdle");
            currentState = ChangelingStates::IDLE_BURIED;
        }
    }
}

void Changeling::UpdateBuriedTravelState(float deltaTime, float distanceToPlayerSq)
{
    if (stateTimer <= 0)
    {
        if (!animComponent->IsFinished())
        {
            float3 resultPos;
            bool posOverPoly        = false;
            const float3 searchArea = {3.0f, 1.0f, 3.0f};

            agentAI->GetClosestPointInNavmesh(spottedLocation, searchArea, posOverPoly, resultPos);

            if (posOverPoly)
            {
                agentAI->SetPosition(resultPos);
            }
            spottedLocation = float3::nan;
            audioComp->StopAudio();
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_BURYUP);
            animComponent->GetAnimationController()->Resume();
        }
        else
        {
            animComponent->UseTrigger("Trigger_BurriedIdle");
            currentState = ChangelingStates::IDLE_BURIED;
        }
    }
}

void Changeling::UpdateIdleVisibleState(float deltaTime, float distanceToPlayerSq, bool lastAttack)
{
    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

    if (ST_StartChase(deltaTime, distanceToPlayerSq, lastAttack)) return;

    animComponent->UseTrigger("Trigger_BuryDown");
    audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_BURYDOWN);

    vfxDigDown->SetEnabled(true);
    vfxDigDown->GetScriptByType<AttackVfxSpritesheet>()->Reset();

    stateTimer   = secondsUntilCompletelyBuried;
    currentState = ChangelingStates::DIG_DOWN_TRANSITION;
}

void Changeling::UpdateChaseState(float deltaTime, float distanceToPlayerSq, bool lastAttack)
{
    if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

    if (!lastAttack && distanceToPlayerSq > rangeAIChase * rangeAIChase)
    {
        agentAI->SetSpeed(0.0f, 10.0f);
        animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
    else
    {
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
        agentAI->SetPathNavigation(character->GetLastPosition());
    }
}

void Changeling::UpdateDashAttackPreparationState(float deltaTime, float distanceToPlayerSq)
{
    agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent->IsFinished())
    {
        Character::Attack(deltaTime);

        weaponCollider->SetEnabled(true);
        dashTrailMeshObject.dashTrailObject->SetEnabled(true);
        dashTrailColliderObject->SetEnabled(true);
        animComponent->UseTrigger("Trigger_Dash");
        audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_DASH);
        for (int i = 0; i < 5; i++)
        {
            vfxDashTrailObjects[i]->GetComponent<MeshComponent*>()->SetEnabled(false);
            vfxDashTrailObjects[i]
                ->GetComponent<ShaderScriptComponent*>()
                ->GetScriptByType<AttackVfxSpritesheet>()
                ->Reset();
        }

        const float3 characterPos = parent->GetGlobalTransform().TranslatePart();
        const float3 offset       = float3::unitY * 0.45f;
        const float3 scale        = vfxDash->GetLocalTransform().ExtractScale();

        const Quat lookRotation =
            Quat::LookAt(float3::unitZ, parent->GetGlobalTransform().WorldZ(), float3::unitY, float3::unitY);

        const Quat verticalCorrection   = Quat::RotateAxisAngle(float3::unitX, 90.0f * (PI / 180));
        const Quat horizontalCorrection = Quat::RotateAxisAngle(float3::unitZ, 90.0f * (PI / 180));

        const Quat finalRotation        = lookRotation * verticalCorrection * horizontalCorrection;

        const float4x4 transform        = float4x4::FromTRS(characterPos + offset, finalRotation, scale);

        const float4x4 parentWS         = parent->GetParentGlobalTransform();
        const float4x4 localTRS         = parentWS.Inverted() * transform;

        vfxDash->SetLocalTransform(localTRS);
        vfxDash->SetEnabled(true);
        vfxDash->GetComponent<MeshComponent*>()->SetEnabled(false);
        vfxDash->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();

        ST_AimNextDashAttack(deltaTime, distanceToPlayerSq);
        currentState = ChangelingStates::DASH_ATTACK;
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
        animComponent->UseTrigger("Trigger_FinishDash");
        currentState              = ChangelingStates::DASH_ATTACK_COOLDOWN;

        const float3 characterPos = parent->GetGlobalTransform().TranslatePart();
        const float3 offset       = float3::unitY * 0.2f;
        const float3 scale        = vfxDropDown->GetLocalTransform().ExtractScale();

        const Quat lookRotation =
            Quat::LookAt(float3::unitZ, parent->GetGlobalTransform().WorldZ(), float3::unitY, float3::unitY);

        const Quat finalRotation = lookRotation;

        const float4x4 transform = float4x4::FromTRS(characterPos + offset, finalRotation, scale);

        const float4x4 parentWS  = parent->GetParentGlobalTransform();
        const float4x4 localTRS  = parentWS.Inverted() * transform;

        vfxDropDown->SetLocalTransform(localTRS);
        vfxDropDown->SetEnabled(true);
        vfxDropDown->GetComponent<MeshComponent*>()->SetEnabled(false);
        vfxDropDown->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }
    else
    {
        const float3 lerpTranslation = (dashStart.TranslatePart() + dashDirection * (distanceFromDashStart / 2.f)) -
                                       parentGO->GetGlobalTransform().TranslatePart();
        const Quat lerpRotation = Quat(dashStart.RotatePart());

        dashTrailMeshObject.dashTrailStartChildMeshObject->SetLocalTransform(float4x4::FromTRS(
            (dashStart.TranslatePart() + dashDirection * distanceFromDashStart) -
                parentGO->GetGlobalTransform().TranslatePart(),
            lerpRotation * Quat::FromEulerXYZ(0, PI / 2.f, 0), float3(1, 1, 1)
        ));
        dashTrailMeshObject.dashTrailMidChildBaseObject->SetLocalTransform(
            float4x4::FromTRS(lerpTranslation, lerpRotation * Quat::FromEulerXYZ(0, PI / 2.f, 0), float3(1, 1, 1))
        );
        dashTrailMeshObject.dashTrailMidChildMeshObject->SetLocalTransform(
            float4x4::FromTRS(float3::zero, Quat::identity, float3(distanceFromDashStart, 1, 1))
        );
        dashTrailMeshObject.dashTrailEndChildMeshObject->SetLocalTransform(float4x4::FromTRS(
            dashStart.TranslatePart() - parentGO->GetGlobalTransform().TranslatePart(),
            lerpRotation * Quat::FromEulerXYZ(0, PI / 2.f, 0), float3(1, 1, 1)
        ));
        dashTrailColliderObject->SetLocalTransform(
            float4x4::FromTRS(lerpTranslation, lerpRotation, float3(1, 1, 1))
        );
        dashAreaCollider->size = float3(.5f, .2f, distanceFromDashStart / 2.f);
        dashAreaCollider->UpdateCollider();
    }
}

void Changeling::UpdateDashAttackCooldownState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent->IsFinished())
    {
        const int bUseAnimation1 = rand() % 5;
        animComponent->UseTrigger(
            bUseAnimation1 == 0   ? "Trigger_Scream"
            : bUseAnimation1 == 1 ? "Trigger_Scream2"
                                  : "Trigger_VisibleIdle"
        );
        vfxDropDown->SetEnabled(false);
    }
    if (stateTimer < 0.f)
    {
        dashTrailMeshObject.dashTrailObject->SetEnabled(false);

        dashTrailColliderObject->SetEnabled(false);

        if (ST_DashAttack(deltaTime, distanceToPlayerSq)) return;

        agentAI->ResetSpeed();

        animComponent->UseTrigger("Trigger_VisibleIdle");
        currentState = ChangelingStates::IDLE_VISIBLE;
    }
}

void Changeling::UpdateBiteAttackState(float deltaTime, float distanceToPlayerSq)
{
    biteVfxTimer -= deltaTime;

    if (biteVfxTimer < 0.0f && biteVfxTimer > -0.3f && vfxBite && !vfxBite->GetEnabled())
    {
        const float3 offset = float3::unitY * 0.5f;
        vfxBite->GetParent()->SetLocalPosition(parent->GetLocalTransform().TranslatePart() + offset);
        vfxBite->SetEnabled(true);
        vfxBite->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }

    if (animComponent->IsFinished())
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
    if (animComponent->IsFinished())
    {
        currentState      = stateAfterDamaged;
        stateAfterDamaged = ChangelingStates::NONE;

        if (currentState == ChangelingStates::IDLE_BURIED)
        {
            characterCollider->SetEnabled(false);
            animComponent->UseTrigger("Trigger_BurriedIdle");
        }
        else // currentState == IDLE_VISIBLE
        {
            dashTrailMeshObject.dashTrailObject->SetEnabled(false);

            dashTrailColliderObject->SetEnabled(false);

            animComponent->UseTrigger("Trigger_VisibleIdle");
        }
    }
}

void Changeling::UpdateDyingState(float deltaTime, float distanceToPlayerSq)
{
    if (animComponent->IsFinished())
    {
        isDead = true;
        parentGO->SetEnabled(false);
    }
}

void Changeling::UpdateHighlightState(float deltaTime, float distanceToPlayerSq)
{
    switch (currentHighlightingState)
    {
    case HighlightingStates::IDLE:
        animComponent->UseTrigger("Trigger_BurriedIdle");
        animComponent->UseTrigger("Trigger_BuryUp");
        audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_BURYUP);
        vfxDigUpRocksObject->SetEnabled(true);
        vfxDigUpRocksObject->GetComponent<AnimationComponent*>()->OnPlay(false, false);
        vfxDigUpHoleObject->SetEnabled(true);
        vfxDigUpHoleObject->GetComponent<AnimationComponent*>()->OnPlay(false, false);
        currentHighlightingState = HighlightingStates::BURY_UP;
        break;
    case HighlightingStates::BURY_UP:
        if (animComponent->IsFinished())
        {
            vfxDigUpRocksObject->SetEnabled(false);
            vfxDigUpHoleObject->SetEnabled(false);

            animComponent->UseTrigger("Trigger_VisibleIdle");
            animComponent->UseTrigger("Trigger_PrepareDash");
            vfxDropDown->SetEnabled(true);
            vfxDropDown->GetComponent<MeshComponent*>()->SetEnabled(false);
            vfxDropDown->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>()->Reset();
            currentHighlightingState = HighlightingStates::DROP_DOWN;
        }
        break;
    case HighlightingStates::DROP_DOWN:
        if (animComponent->IsFinished())
        {
            vfxDropDown->SetEnabled(false);

            stateTimer = highlightDuration;
            animComponent->UseTrigger("Trigger_Dash");
            animComponent->UseTrigger("Trigger_Wiggle");
            currentHighlightingState = HighlightingStates::WIGGLE;
        }
        break;
    case HighlightingStates::WIGGLE:
        if (stateTimer <= 0.f)
        {
            animComponent->UseTrigger("Trigger_FinishDash");
            currentHighlightingState = HighlightingStates::STAND_UP;
        }
        break;
    case HighlightingStates::STAND_UP:
        if (animComponent->IsFinished())
        {
            animComponent->UseTrigger("Trigger_VisibleIdle");
            animComponent->UseTrigger("Trigger_BuryDown");
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_BURYDOWN);
            vfxDigDown->SetEnabled(true);
            vfxDigDown->GetScriptByType<AttackVfxSpritesheet>()->Reset();
            currentHighlightingState = HighlightingStates::BURY_DOWN;
        }
        break;
    case HighlightingStates::BURY_DOWN:
        highlightDigTimer += deltaTime;

        if (highlightDigTimer > 0.5f && highlightDigTimer < 0.8f && !vfxDig->GetEnabled())
        {
            const float3 dir    = parent->GetGlobalTransform().WorldZ();
            const float3 offset = float3(dir.x * 0.8f, 0.25f, dir.z * 0.8f);
            vfxDig->GetParent()->SetLocalPosition(
                parent->GetGlobalTransform().TranslatePart() + offset -
                parent->GetParentGlobalTransform().TranslatePart()
            );
            vfxDig->SetEnabled(true);
            vfxDig->GetScriptByType<AttackVfxSpritesheet>()->Reset();
        }

        if (animComponent->IsFinished())
        {
            vfxDigDown->SetEnabled(false);
            animComponent->UseTrigger("Trigger_BurriedIdle");
            currentHighlightingState = HighlightingStates::IDLE;
            currentState             = ChangelingStates::IDLE_BURIED;
        }
        break;
    }
}

bool Changeling::ST_BuryUp(float deltaTime, float distanceToPlayerSq, bool lastAttack)
{
    // Check preconditions
    if (!lastAttack && currentState != ChangelingStates::IDLE_BURIED) return false;
    if (!lastAttack && !spottedLocation.IsFinite()) return false;

    // Implement state transition
    if (stateTimer <= 0.f)
    {
        vfxDigUpRocksObject->SetEnabled(true);
        vfxDigUpRocksObject->GetComponent<AnimationComponent*>()->OnPlay(false, false);
        vfxDigUpHoleObject->SetEnabled(true);
        vfxDigUpHoleObject->GetComponent<AnimationComponent*>()->OnPlay(false, false);
        characterCollider->SetEnabled(true);
        animComponent->UseTrigger("Trigger_BuryUp");
        audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_BURYUP);
        currentState = ChangelingStates::DIG_UP_TRANSITION;
    }

    return true;
}

bool Changeling::ST_StartChase(float deltaTime, float distanceToPlayerSq, bool lastAttack)
{
    // Check preconditions
    if (currentState != ChangelingStates::IDLE_VISIBLE) return false;
    if (!lastAttack && distanceToPlayerSq > rangeAIChase * rangeAIChase) return false;

    // Implement state transition
    const bool bUseAnimation1 = rand() % 2;
    animComponent->UseTrigger(bUseAnimation1 ? "Trigger_Run" : "Trigger_Run2");

    agentAI->ResetSpeed();
    agentAI->SetSpeed(chaseSpeed, chaseAcceleration);

    currentState = ChangelingStates::CHASE;

    return true;
}

bool Changeling::ST_Damaged()
{
    if (currentState != ChangelingStates::DAMAGED)
    {
        if (currentState == ChangelingStates::PEEK || currentState == ChangelingStates::BITE_ATTACK)
            stateAfterDamaged = ChangelingStates::IDLE_BURIED;
        else stateAfterDamaged = ChangelingStates::IDLE_VISIBLE;

        currentState = ChangelingStates::DAMAGED;

        agentAI->ResetSpeed();
        agentAI->SetSpeed(0, 10);

        weaponCollider->SetEnabled(false);

        const bool bUseAnimation1 = rand() % 2;
        if (!animComponent->UseTrigger(bUseAnimation1 ? "Trigger_Hit" : "Trigger_Hit2"))
            animComponent->UseTrigger("Trigger_HitUnderground");
    }
    audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_HURT);

    return true;
}

bool Changeling::ST_Peek(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (currentState != ChangelingStates::IDLE_BURIED) return false;

    // Implement state transition
    const int randomValue = max(1, (int)round(1.0f / (peekChancePerSecond * deltaTime)));

    // Random value = 1 only if fps are too high -> Ignore those frames then
    if (randomValue == 1 || rand() % randomValue != 0) return false; // Only peek randomly

    characterCollider->SetEnabled(true);
    agentAI->SetSpeed(0.0f, 10.0f);
    animComponent->UseTrigger("Trigger_Peek");
    currentState = ChangelingStates::PEEK;

    return true;
}

bool Changeling::ST_DashAttack(float deltaTime, float distanceToPlayerSq)
{
    // Check preconditions
    if (distanceToPlayerSq > rangeAIAttack * rangeAIAttack) return false;
    if (currentState != ChangelingStates::CHASE && currentState != ChangelingStates::DASH_ATTACK_COOLDOWN) return false;

    // Reset parameters
    dashStart = float4x4();
    dashStart.SetTranslatePart(float3::inf);
    dashStart.SetRotatePart(Quat::nan);

    // Implement state transition

    animComponent->UseTrigger("Trigger_PrepareDash");
    currentState = ChangelingStates::DASH_ATTACK_PREPARATION;

    agentAI->SetLookForward(false);
    agentAI->SetSpeed(0.0f, 10.0f);
    agentAI->ResetSpeed();
    agentAI->SetLookForward(true);

    return true;
}

bool Changeling::ST_AimNextDashAttack(float deltaTime, float distanceToPlayerSq)
{
    CalculateDashTargetPoint(character->GetLastPosition(), dashTarget);

    if (minDashDistance > activeDashRange)
    {
        isAttacking = false;
        stateTimer  = attackCooldown;
        animComponent->UseTrigger("Trigger_FinishDash");
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
    if (currentState != ChangelingStates::IDLE_BURIED && currentState != ChangelingStates::BITE_ATTACK_COOLDOWN)
        return false;

    // Implement state transition
    characterCollider->SetEnabled(true);
    agentAI->SetSpeed(0.0f, 10.0f);
    animComponent->UseTrigger("Trigger_Bite");
    currentState = ChangelingStates::BITE_ATTACK;

    biteVfxTimer = 0.4f;
    weaponCollider->SetEnabled(true);

    Character::Attack(deltaTime);

    audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_POOKA_ATTACK);

    return true;
}

void Changeling::ValidateSetup()
{
    isSetupCorrectly = true;

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

    // Validate parents children game objects
    for (const UID childUID : parentGO->GetChildren())
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child == nullptr)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Parent child game object is nullptr")
            return;
        }

        if (child->GetName() == dashTrailObjectName)
        {
            ChangelingDashTrailContainer container = ChangelingDashTrailContainer();
            container.dashTrailObject              = child;

            for (const UID dashTrailChildUID : child->GetChildren())
            {
                GameObject* dashTrailChild =
                    AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(dashTrailChildUID);
                if (dashTrailChild == nullptr)
                {
                    isSetupCorrectly = false;
                    GLOG("[ERROR] Dash trail game object is nullptr")
                    return;
                }

                if (dashTrailChild->GetName() == dashTrailStartMeshName)
                {
                    container.dashTrailStartChildMeshObject = dashTrailChild;
                    for (const UID dashTrailVFXUID : dashTrailChild->GetChildren())
                    {
                        GameObject* dashTrailVFXChild =
                            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(dashTrailVFXUID);
                        if (dashTrailVFXChild == nullptr)
                        {
                            isSetupCorrectly = false;
                            GLOG("[ERROR] Dash trail vfx child game object is nullptr")
                            return;
                        }
                        if (dashTrailVFXChild->GetName() == vfxDashTrailName)
                        {
                            if (dashTrailVFXChild->GetComponent<ShaderScriptComponent*>() != nullptr &&
                                dashTrailVFXChild->GetComponent<ShaderScriptComponent*>()
                                        ->GetScriptByType<AttackVfxSpritesheet>() != nullptr)
                            {
                                vfxDashTrailObjects.push_back(dashTrailVFXChild);
                            }
                        }
                    }
                }
                else if (dashTrailChild->GetName() == dashTrailMidBaseName)
                {
                    container.dashTrailMidChildBaseObject = dashTrailChild;
                    for (const UID dashTrailVFXUID : dashTrailChild->GetChildren())
                    {
                        GameObject* dashTrailVFXChild =
                            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(dashTrailVFXUID);
                        if (dashTrailVFXChild == nullptr)
                        {
                            isSetupCorrectly = false;
                            GLOG("[ERROR] Dash trail vfx child game object is nullptr")
                            return;
                        }
                        if (dashTrailVFXChild->GetName() == vfxDashTrailName)
                        {
                            if (dashTrailVFXChild->GetComponent<ShaderScriptComponent*>() != nullptr &&
                                dashTrailVFXChild->GetComponent<ShaderScriptComponent*>()
                                        ->GetScriptByType<AttackVfxSpritesheet>() != nullptr)
                            {
                                vfxDashTrailObjects.push_back(dashTrailVFXChild);
                            }
                        }
                        else if (dashTrailVFXChild->GetName() == dashTrailMidMeshName)
                        {
                            container.dashTrailMidChildMeshObject = dashTrailVFXChild;
                        }
                    }
                }
                else if (dashTrailChild->GetName() == dashTrailEndMeshName)
                {
                    container.dashTrailEndChildMeshObject = dashTrailChild;
                    for (const UID dashTrailVFXUID : dashTrailChild->GetChildren())
                    {
                        GameObject* dashTrailVFXChild =
                            AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(dashTrailVFXUID);
                        if (dashTrailVFXChild == nullptr)
                        {
                            isSetupCorrectly = false;
                            GLOG("[ERROR] Dash trail vfx child game object is nullptr")
                            return;
                        }
                        if (dashTrailVFXChild->GetName() == vfxDashTrailName)
                        {
                            if (dashTrailVFXChild->GetComponent<ShaderScriptComponent*>() != nullptr &&
                                dashTrailVFXChild->GetComponent<ShaderScriptComponent*>()
                                        ->GetScriptByType<AttackVfxSpritesheet>() != nullptr)
                            {
                                vfxDashTrailObjects.push_back(dashTrailVFXChild);
                            }
                        }
                    }
                }
            }

            if (container.dashTrailStartChildMeshObject == nullptr)
            {
                isSetupCorrectly = false;
                GLOG("[ERROR] Dash trail start child game object is nullptr")
                return;
            }

            if (container.dashTrailMidChildBaseObject == nullptr)
            {
                isSetupCorrectly = false;
                GLOG("[ERROR] Dash trail mid child base game object is nullptr")
                return;
            }

            if (container.dashTrailMidChildMeshObject == nullptr)
            {
                isSetupCorrectly = false;
                GLOG("[ERROR] Dash trail mid child game object is nullptr")
                return;
            }

            if (container.dashTrailEndChildMeshObject == nullptr)
            {
                isSetupCorrectly = false;
                GLOG("[ERROR] Dash trail end child game object is nullptr")
                return;
            }

            dashTrailMeshObject = container;
        }
        else if (child->GetName() == dashTrailCollisionName)
        {
            dashTrailColliderObject = child;
        }
        else if (child->GetName() == vfxDropDownName)
        {
            vfxDropDown = child;
        }
        else if (child->GetName() == vfxBiteName)
        {
            vfxBite = child->GetComponent<ShaderScriptComponent*>();
        }
        else if (child->GetName() == vfxDashName)
        {
            vfxDash = child;
        }
        else if (child->GetName() == vfxDigName)
        {
            vfxDig = child->GetComponent<ShaderScriptComponent*>();
        }
    }

    CubeColliderComponent* cCComponent = dashTrailColliderObject->GetComponent<CubeColliderComponent*>();
    if (cCComponent == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] DashTrailColliderObject does not contain a cube collider")
        return;
    }

    dashAreaCollider = cCComponent;

    // VFX validation

    // Validate children game objects
    for (const UID childUID : parent->GetChildren())
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child == nullptr)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Child game object is nullptr")
            return;
        }

        if (child->GetName() == vfxDigUpRocksName)
        {
            vfxDigUpRocksObject = child;
        }
        else if (child->GetName() == vfxDigUpHoleName)
        {
            vfxDigUpHoleObject = child;
        }
        else if (child->GetName() == vfxDigDownName)
        {
            vfxDigDown = child->GetComponent<ShaderScriptComponent*>();
        }
    }

    if (vfxDigUpRocksObject == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX dig up rocks game object not found")
        return;
    }

    if (vfxDigUpRocksObject->GetComponent<AnimationComponent*>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX dig up rocks game object has no animation component")
        return;
    }

    if (vfxDigUpHoleObject == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX dig up hole game object not found")
        return;
    }

    if (vfxDigUpHoleObject->GetComponent<AnimationComponent*>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX dig up hole game object has no animation component")
        return;
    }

    if (vfxDashTrailObjects.size() < 20)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Not enough vfx dash trail objects found")
        return;
    }

    if (vfxDropDown == nullptr || vfxDropDown->GetComponent<ShaderScriptComponent*>() == nullptr ||
        vfxDropDown->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX drop down game object not found or setup incorrectly")
        return;
    }

    if (vfxDash == nullptr || vfxDash->GetComponent<ShaderScriptComponent*>() == nullptr ||
        vfxDash->GetComponent<ShaderScriptComponent*>()->GetScriptByType<AttackVfxSpritesheet>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX dash game object not found or setup incorrectly")
        return;
    }

    if (vfxDigDown == nullptr || vfxDigDown->GetScriptByType<AttackVfxSpritesheet>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX dig down game object not found or setup incorrectly")
        return;
    }

    if (vfxBite == nullptr || vfxBite->GetScriptByType<AttackVfxSpritesheet>() == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] VFX bite game object not found or setup incorrectly")
        return;
    }

    // Audio
    audioComp = parent->GetComponent<AudioSourceComponent*>();
    if (audioComp == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Script parent does not contain an audio component")
        return;
    }
}

void Changeling::RenderDebugVisuals()
{
    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life           = "Health: " + std::to_string(currentHealth);
        const std::string animState      = "Anim state: " + stateName.GetString();
        const std::string characterState = "Character state: " + std::to_string((int)currentState);
        const std::string setupState     = isSetupCorrectly ? "Setup valid" : "Setup invalid";

        std::vector<std::pair<std::string, float2>> logs {
            {life,               float2(-50.0f,  -140.0f)},
            {animState,          float2(-80.0f,  -160.0f)},
            {characterState,     float2(-100.0f, -180.0f)},
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
