#include "pch.h"

#include "Application.h"
#include "MirageBossDash.h"
#include "CameraComponent.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "Globals.h"
#include "ResourceStateMachine.h"
#include "ScriptComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

MirageBossDash::MirageBossDash(GameObject* parent)
    : Character(parent, 60, 1, 0.5f, 1.0f, 1.0f, 3.0f, 15.0f, 20.0f, CharacterType::Boss)
{
    fields.push_back({InspectorField::FieldType::Text, (void*)"Ferdiad specific"});
    fields.push_back({InspectorField::FieldType::Text, (void*)"Colliders"});
}

bool MirageBossDash::Init()
{
    Character::Init();

    return true;
}

void MirageBossDash::Update(float deltaTime)
{
    Character::Update(deltaTime);

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string animState   = "Anim state: " + stateName.GetString();
        const std::string logicAction = "Action: " + std::string(GetActionName());
        const std::string logicState  = "State: " + std::string(GetStateName());

        std::vector<std::pair<std::string, float2>> logs {
            {animState,   float2(-80.0f, -160.0f)},
            {logicAction, float2(-80.0f, -180.0f)},
            {logicState,  float2(-80.0f, -200.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.5f, 0.0f));
    }
}

void MirageBossDash::HandleState(float deltaTime)
{
  

    switch (currentState)
    {
    case BossDashStates::Idle:
        Idle();
        break;

    case BossDashStates::OverheadStrike:
        OverheadStrike(deltaTime);
        break;
    }
}


void MirageBossDash::Idle()
{
    if (stateEnter)
    {

        stateEnter    = false;
        currentAction = BossDashActions::Idle;
        doIdle        = false;

        if (animComponent) animComponent->UseTrigger("Idle");
    }
}

void MirageBossDash::OverheadStrike(float deltaTime)
{

    if (stateEnter)
    {
        stateEnter        = false;
        actionTriggerDone = false;
    }

    switch (currentAction)
    {

    case BossDashActions::Dash:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Dash");
            StartDash();
        }

        if (isDashing) Dash(deltaTime);
        else
        {
            actionTriggerDone = false;
        }

        break;
    }
}

void MirageBossDash::StartDash()
{
    isDashing        = true;

    float3 bossPos   = parent->GetGlobalTransform().TranslatePart();

    bossPos.y        = 0.0f;
    dashEnd.y      = 0.0f;

    dashDistance     = (dashEnd - bossPos).Length();
    dashDirection    = (dashEnd - bossPos).Normalized();

    GLOG("Distance: %.2f", dashDistance);
    GLOG("Direction: %.2f %.2f %.2f", dashDirection.x, dashDirection.y, dashDirection.z);

    dashSpeed         = dashDistance / dashDuration;
    dashTimeRemaining = dashDuration;

    dashStartPosLocal = parent->GetLocalTransform().TranslatePart();

    GLOG("Speed: %.2f", dashSpeed);
}

void MirageBossDash::Dash(float deltaTime)
{
    dashTimeRemaining -= deltaTime;
    if (dashTimeRemaining < 0.0f) dashTimeRemaining = 0.0f;

    float elapsedTime       = dashDuration - dashTimeRemaining;
    float offsetDist        = dashSpeed * elapsedTime;

    float3 horizontalOffset = dashDirection * offsetDist;
    float originalY         = dashStartPosLocal.y;

    float3 newPos           = dashStartPosLocal + float3(horizontalOffset.x, 0.0f, horizontalOffset.z);
    newPos.y                = originalY;

    parent->SetLocalPosition(newPos);

    if (dashTimeRemaining <= 0.0f)
    {
        isDashing = false;
        parent->SetLocalPosition(dashStartPosLocal + float3(horizontalOffset.x, 0.0f, horizontalOffset.z));
    }
}


const char* MirageBossDash::GetStateName() const
{
    switch (currentState)
    {
    case BossDashStates::None:
        return "None";

    case BossDashStates::Idle:
        return "Idle";

    case BossDashStates::OverheadStrike:
        return "OverheadStrike";

    default:
        return "ERROR: NO STATE";
    }
}

const char* MirageBossDash::GetActionName() const
{
    switch (currentAction)
    {
    case BossDashActions::Idle:
        return "Idle";
    case BossDashActions::Dash:
        return "Dash";
    default:
        return "ERROR: NO ACTION";
    }
}