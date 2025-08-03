#include "pch.h"

#include "Application.h"
#include "Boss.h"
#include "CameraComponent.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "Globals.h"
#include "ResourceStateMachine.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

Boss::Boss(GameObject* parent) : Character(parent, 60, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 15.0f, CharacterType::Boss)
{
}

bool Boss::Init()
{
    Character::Init();

    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("AIAgent component not found for Boss")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    rng         = std::mt19937(std::random_device {}());
    uniformDist = std::uniform_int_distribution<int>(0, 100);

    return true;
}

void Boss::Update(float deltaTime)
{
    if (agentAI == nullptr) return;
    Character::Update(deltaTime);

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life        = "Health: " + std::to_string(currentHealth);
        const std::string animState   = "Anim state: " + stateName.GetString();
        const std::string logicAction = "Action: " + std::string(GetActionName());
        const std::string logicState  = "State: " + std::string(GetStateName());

        std::vector<std::pair<std::string, float2>> logs {
            {life,        float2(-50.0f, -140.0f)},
            {animState,   float2(-80.0f, -160.0f)},
            {logicAction, float2(-80.0f, -180.0f)},
            {logicState,  float2(-80.0f, -200.0f)},
        };

        RenderDebug(logs, float3(1.0f, 0.5f, 0.0f));
    }
}

void Boss::OnDeath()
{
    // TODO: include death sound for the character
    // TODO: animation and particles
    parent->SetEnabled(false);
}

void Boss::OnDamageTaken(int amount)
{
    // TODO: play boss take damage sound
    // TODO: particles? and animation
}

void Boss::HandleState(float deltaTime)
{
    switch (currentState)
    {
    case BossStates::Idle:
        Idle();
        break;

    case BossStates::Taunt:
        Taunt();
        break;

    case BossStates::ShieldStrikes:
        ShieldStrikes(deltaTime);
        break;

    case BossStates::OverheadStrike:
        OverheadStrike(deltaTime);
        break;

    case BossStates::Mirage:
        Mirage();
        break;

    case BossStates::WaterSpouts:
        break;
    }
}

void Boss::UpdateTimers(float deltaTime)
{
    Character::UpdateTimers(deltaTime);
}

void Boss::ChooseNextState()
{
    int shieldStrikesRate  = 0;
    int overheadStrikeRate = 0;

    // Set states ratio depending on distance
    switch (CheckDistanceWithPlayer())
    {
    case PlayerDistances::Close:
        shieldStrikesRate = 100;
        // shieldStrikesRate  = 75;
        // overheadStrikeRate = 100;
        break;

    case PlayerDistances::Medium:
        shieldStrikesRate = 100;
        // shieldStrikesRate  = 50;
        // overheadStrikeRate = 100;
        break;

    case PlayerDistances::Far:
        doTaunt = true;
        break;
    }

    int num = uniformDist(rng);
    if (activateMirage)
    {
        GLOG("Mirage chosen")
        currentState = BossStates::Mirage;
    }
    else if (doTaunt)
    {
        GLOG("Taunt chosen")
        currentState = BossStates::Taunt;
    }
    else
    {
        if (num <= shieldStrikesRate)
        {
            GLOG("ShieldStrikes chosen")
            currentState = BossStates::ShieldStrikes;
        }
        else if (num <= overheadStrikeRate)
        {
            GLOG("OverheadStrike chosen")
            currentState = BossStates::OverheadStrike;
        }
    }

    stateEnter = true;
}

void Boss::Idle()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Idle");

        // TODO: Randomize the idle duration
        // agentAI->SetSpeed(0.0f, 10.0f);

        stateEnter    = false;
        currentAction = BossActions::Idle;

        if (animComponent) animComponent->UseTrigger("Idle");
    }

    ChooseNextState();
}

void Boss::Taunt()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Taunt");

        stateEnter    = false;
        doTaunt       = false;
        currentAction = BossActions::Taunt;

        if (animComponent) animComponent->UseTrigger("Taunt");
    }

    if (animComponent && animComponent->IsFinished()) Idle();
    else ChooseNextState();
}

void Boss::ShieldStrikes(float deltaTime)
{
    if (!weaponCollider) return;

    if (stateEnter)
    {
        GLOG("[BOSS] - Shield Strikes");

        stateEnter    = false;

        currentAction = BossActions::Combo1;
        agentAI->PauseMovement();
    }

    switch (currentAction)
    {
    case BossActions::Combo1:
        if (!actionTriggerDone)
        {
            attackHitboxDelay = 1.0f;
            Character::Attack(deltaTime);
            if (animComponent) animComponent->UseTrigger("Combo1");
            actionTriggerDone = true;
        }

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Combo2;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Combo2:
        if (!actionTriggerDone)
        {
            attackHitboxDelay = 0.7f;
            Character::Attack(deltaTime);
            if (animComponent) animComponent->UseTrigger("Combo2");
            actionTriggerDone = true;
        }

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Combo3;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Combo3:
        if (!actionTriggerDone)
        {
            attackHitboxDelay = 0.8f;
            Character::Attack(deltaTime);
            if (animComponent) animComponent->UseTrigger("Combo3");
            actionTriggerDone = true;
        }

        if (animComponent && animComponent->IsFinished())
        {
            actionTriggerDone = false;

            isAttacking       = false;
            attackCdTimer     = attackCooldown;
            agentAI->ResumeMovement();
            ChooseNextState();
        }
        break;
    default:
        GLOG("Error: ShieldStrikes");
        break;
    }

    // Enable hitbox when animation hits
    if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
        attackTimer <= attackHitboxDelay + attackHitboxDuration)
    {
        weaponCollider->SetEnabled(true);
    }
    else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
    {
        weaponCollider->SetEnabled(false);
    }
}

void Boss::OverheadStrike(float deltaTime)
{
    if (!weaponCollider) return;

    if (stateEnter)
    {
        GLOG("[BOSS] - Overhead Strikes");

        stateEnter = false;
        agentAI->PauseMovement();
        currentAction = BossActions::Prepare;
    }

    switch (currentAction)
    {
    case BossActions::Prepare:
        if (animComponent) animComponent->UseTrigger("Prepare");
        if (animComponent && animComponent->IsFinished()) currentAction = BossActions::Jump;
        break;

    case BossActions::Jump:
        if (animComponent) animComponent->UseTrigger("Jump");
        if (animComponent && animComponent->IsFinished()) currentAction = BossActions::Dash;
        break;

    case BossActions::Dash:
        if (animComponent) animComponent->UseTrigger("Dash");
        if (animComponent && animComponent->IsFinished()) currentAction = BossActions::Land;
        break;

    case BossActions::Land:
        if (animComponent) animComponent->UseTrigger("Land");
        if (animComponent && animComponent->IsFinished()) currentAction = BossActions::Attack;
        break;

    case BossActions::Attack:
        Character::Attack(deltaTime);
        if (animComponent) animComponent->UseTrigger("Attack");

        // Enable hitbox when animation hits
        if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
            attackTimer <= attackHitboxDelay + attackHitboxDuration)
        {
            weaponCollider->SetEnabled(true);
        }
        else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
        {
            weaponCollider->SetEnabled(false);
        }

        if (animComponent && animComponent->IsFinished())
        {
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            currentAction = BossActions::Recover;
        }
        break;

    case BossActions::Recover:
        if (animComponent) animComponent->UseTrigger("Recover");
        if (animComponent && animComponent->IsFinished())
        {
            agentAI->ResumeMovement();
            ChooseNextState();
        }

        break;

    default:
        GLOG("Error: OverheadStrike")
        break;
    }
}

void Boss::Mirage()
{
}

const char* Boss::GetStateName() const
{
    switch (currentState)
    {
    case BossStates::None:
        return "None";

    case BossStates::Idle:
        return "Idle";

    case BossStates::Taunt:
        return "Taunt";

    case BossStates::ShieldStrikes:
        return "ShieldStrikes";

    case BossStates::OverheadStrike:
        return "OverheadStrike";

    case BossStates::Mirage:
        return "Mirage";

    case BossStates::WaterSpouts:
        return "WaterSpouts";

    default:
        return "ERROR: NO STATE";
    }
}

const char* Boss::GetActionName() const
{
    switch (currentAction)
    {
    case BossActions::Idle:
        return "Idle";

    case BossActions::Taunt:
        return "Taunt";

    case BossActions::Combo1:
        return "Combo1";

    case BossActions::Combo2:
        return "Combo2";

    case BossActions::Combo3:
        return "Combo3";

    case BossActions::Prepare:
        return "Prepare";

    case BossActions::Jump:
        return "Jump";

    case BossActions::Dash:
        return "Dash";

    case BossActions::Land:
        return "Land";

    case BossActions::Attack:
        return "Attack";

    case BossActions::Recover:
        return "Recover";

    case BossActions::Mirage:
        return "Mirage";

    case BossActions::WaterSpouts:
        return "WaterSpouts";

    default:
        return "ERROR: NO ACTION";
    }
}