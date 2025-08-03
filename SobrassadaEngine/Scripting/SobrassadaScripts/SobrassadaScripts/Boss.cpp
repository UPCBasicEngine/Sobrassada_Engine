#include "pch.h"

#include "Application.h"
#include "Boss.h"
#include "BossMirage.h"
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

Boss::Boss(GameObject* parent) : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 10.0f, CharacterType::Boss)
{
    health = 100;

    fields.push_back({"Boss Max Health", InspectorField::FieldType::Int, &health, 0, 1000});
    fields.push_back({"Mirage1 Threshhold", InspectorField::FieldType::Int, &health, 0, 1000});
    fields.push_back({"Mirage2 Threshhold", InspectorField::FieldType::Int, &health, 0, 1000});
    fields.push_back({"Mirage3 Threshhold", InspectorField::FieldType::Int, &health, 0, 1000});
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

    rng                 = std::mt19937(std::random_device {}());
    uniformDist         = std::uniform_int_distribution<int>(0, 100);

    GameObject* arenaGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("Arena");
    if (arenaGO)
    {

        if (scriptComp && scriptComp->GetScriptByType<Mirage>())
            ScriptComponent* sc = arenaGO->GetComponent<ScriptComponent*>();
        if (sc && sc->GetScriptByType<BossMirage>())
            ;
    }
    if (!bossMirage)
    {
        GLOG("BossMirage not found! Mirage sequences will not trigger.");
    }

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
    health = health - amount;
    switch (currentMirage)
    {
    case 1:
        if (health < mirage1)
        {
        }
    }
    // update healthbar
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

    case BossStates::ShieldStrikes:
        ShieldStrikes(deltaTime);
        break;

    case BossStates::OverheadStrike:
        OverheadStrike();
        break;

    case BossStates::Mirage:
        Mirage();
        break;

    case BossStates::WaterSpouts:
        break;
    }

    if (animComponent && animComponent->IsFinished())
    {
        animComponent->UseTrigger("Idle");
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
        shieldStrikesRate  = 75;
        overheadStrikeRate = 100;
        break;

    case PlayerDistances::Medium:
        shieldStrikesRate  = 50;
        overheadStrikeRate = 100;
        break;

    case PlayerDistances::Far:
        shieldStrikesRate  = 25;
        overheadStrikeRate = 100;
        break;
    }

    int num = uniformDist(rng);
    if (activateMirage)
    {
        currentState = BossStates::Mirage;
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
        stateEnter = false;
        agentAI->SetSpeed(0.0f, 10.0f);
        if (animComponent) animComponent->UseTrigger("Idle");
    }

    ChooseNextState();
}

void Boss::ShieldStrikes(float deltaTime)
{
    if (!weaponCollider) return;

    if (!isAttacking)
    {
        GLOG("[BOSS] - Shield Strikes");

        if (animComponent) animComponent->UseTrigger("ShieldStrikes");
        Character::Attack(deltaTime);
        agentAI->PauseMovement();
    }
    else
    {
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

        // Reset attack state
        if (attackTimer >= attackDuration)
        {
            isAttacking   = false;
            attackCdTimer = attackCooldown;
            agentAI->ResumeMovement();
            ChooseNextState();
        }
    }
}

void Boss::OverheadStrike()
{
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
        break;

    case BossStates::ShieldStrikes:
        return "ShieldStrikes";
        break;

    case BossStates::OverheadStrike:
        return "OverheadStrike";
        break;

    case BossStates::WaterSpouts:
        return "WaterSpouts";
        break;

    case BossStates::Mirage:
        return "Mirage";
        break;

    default:
        return "ERROR: NO STATE";
        break;
    }
}

const char* Boss::GetActionName() const
{
    switch (currentAction)
    {
    case BossActions::Idle:
        return "Idle";
        break;

    case BossActions::ShieldStrikes:
        return "ShieldStrikes";
        break;

    case BossActions::OverheadStrike:
        return "OverheadStrike";
        break;

    case BossActions::WaterSpouts:
        return "WaterSpouts";
        break;

    case BossActions::Mirage:
        return "Mirage";
        break;

    default:
        return "ERROR: NO ACTION";
        break;
    }
}