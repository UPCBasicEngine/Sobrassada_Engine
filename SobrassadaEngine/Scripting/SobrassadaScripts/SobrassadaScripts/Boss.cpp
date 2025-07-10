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

Boss::Boss(GameObject* parent) : Character(parent, 3, 1, 0.5f, 1.0f, 1.0f, 2.0f, 10.0f, 10.0f, CharacterType::Boss)
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
    // TODO: play soldier take damage sound
    // TODO: particles? and animation
}

void Boss::HandleState(float deltaTime)
{
    switch (currentAction)
    {
    case BossActions::Idle:
        Idle();
        break;

    case BossActions::Approach:
        Approach();
        break;

    case BossActions::Surround:
        Surround();
        break;

    case BossActions::JumpAway:
        JumpAway();
        break;

    case BossActions::Chase:
        Chase();
        break;

    case BossActions::ShieldStrikes:
        ShieldStrikes(deltaTime);
        break;

    case BossActions::ShieldThrow:
        ShieldThrow();
        break;

    case BossActions::WaterSpouts:
        break;

    case BossActions::Mirage:
        break;

    case BossActions::OverheadStrike:
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

    if (currentState == BossStates::Movement) movementTimer -= deltaTime;
}

void Boss::ChooseNextState()
{
    // TODO: This will grow and be modified dynamycally for each phase, but for now

    int movementRate = 0;
    int meleeRate    = 0;
    int rangedRate   = 0;

    // Set states ratio depending on distance
    switch (CheckDistanceWithPlayer())
    {
    case PlayerDistances::Close:
        meleeRate    = 50;
        movementRate = 25;
        rangedRate   = 25;
        break;

    case PlayerDistances::Medium:
        meleeRate    = 35;
        movementRate = 30;
        rangedRate   = 35;
        break;

    case PlayerDistances::Far:
        meleeRate    = 25;
        movementRate = 25;
        rangedRate   = 50;
    }

    int num = uniformDist(rng);
    if (num < movementRate)
    {
        currentState = BossStates::Movement;
        if (uniformDist(rng) < 30)
        {
            currentAction = BossActions::Idle;
        }
        else
        {
            switch (CheckDistanceWithPlayer())
            {
            case PlayerDistances::Close:
                currentAction = BossActions::JumpAway;
                break;
            case PlayerDistances::Medium:
                currentAction = BossActions::Surround;
                break;
            case PlayerDistances::Far:
                currentAction = BossActions::Approach;
                break;
            }
        }
    }
    else if (num < movementRate + meleeRate)
    {
        currentState  = BossStates::ShieldStrikes;
        currentAction = BossActions::Chase;
    }
    else
    {
        currentState = BossStates::ShieldThrow;
        if (GetDistanceFromPlayer() < shieldThrowMinDistance) currentAction = BossActions::JumpAway;
        else currentAction = BossActions::ShieldThrow;
    }

    stateEnter = true;
}

void Boss::Idle()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Idle");

        // TODO: Randomize the idle duration
        stateEnter    = false;
        movementTimer = 4.0f;
        agentAI->SetSpeed(0.0f, 10.0f);
        // if (animComponent) animComponent->UseTrigger("Idle");
    }

    if (movementTimer <= 0.0f)
    {
        ChooseNextState();
    }
}

void Boss::Approach()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Approach");

        // TODO: Randomize the idle duration
        stateEnter    = false;
        movementTimer = 4.0f;
        agentAI->SetSpeed(walkSpeed, 10.0f);
        // if (animComponent) animComponent->UseTrigger("Walk");
    }

    agentAI->SetPathNavigation(character->GetLastPosition());

    if (CheckDistanceWithPlayer() == PlayerDistances::Close || movementTimer <= 0.0f)
    {
        ChooseNextState();
    }
}

void Boss::Surround()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Surround");

        // TODO: Randomize the idle duration
        stateEnter    = false;
        movementTimer = 4.0f;
        agentAI->SetSpeed(walkSpeed, 10.0f);
        // if (animComponent) animComponent->UseTrigger("Walk");
    }

    // TODO: Move surrounding the player, along the perpendicular of the vector between both

    if (movementTimer <= 0.0f)
    {
        ChooseNextState();
    }
}

void Boss::JumpAway()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Jump Back");

        // TODO: Randomize the idle duration
        stateEnter    = false;
        movementTimer = 4.0f;
        // if (animComponent) animComponent->UseTrigger("Idle");
    }

    // TODO: Find a distant position valid in the navmesh and jump to it

    if (movementTimer <= 0.0f)
    {
        ChooseNextState();
    }
}

void Boss::Chase()
{
    if (!character) return;

    if (stateEnter)
    {
        GLOG("[BOSS] - Chase");

        stateEnter = false;
        agentAI->SetSpeed(chaseSpeed, 10.0f);
        // if (animComponent) animComponent->UseTrigger("Run");
    }

    // TODO: Maybe has time limit? If you are super far he stops chasing and never does the attack
    agentAI->SetPathNavigation(character->GetLastPosition());

    const float distanceToPlayer = GetDistanceFromPlayer();
    if (currentState == BossStates::ShieldStrikes && distanceToPlayer < shieldStrikesRange)
        currentAction = BossActions::ShieldStrikes;
    else if (currentState == BossStates::OverheadStrike && distanceToPlayer < overheadStrikeRange)
        currentAction = BossActions::OverheadStrike;
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

void Boss::ShieldThrow()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Shield Throw");

        stateEnter = false;
        // if (animComponent) animComponent->UseTrigger("ShieldThrow");
    }
    else
    {
        ChooseNextState();
    }
}

const char* Boss::GetStateName() const
{
    switch (currentState)
    {
    case BossStates::Movement:
        return "Movement";
        break;

    case BossStates::ShieldStrikes:
        return "ShieldStrikes";
        break;

    case BossStates::ShieldThrow:
        return "ShieldThrow";
        break;

    case BossStates::WaterSpouts:
        return "WaterSpouts";
        break;

    case BossStates::OverheadStrike:
        return "OverheadStrike";
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

    case BossActions::Approach:
        return "Approach";
        break;

    case BossActions::JumpAway:
        return "Jump Away";
        break;

    case BossActions::Surround:
        return "Surround";
        break;

    case BossActions::Chase:
        return "Chase";
        break;

    case BossActions::ShieldStrikes:
        return "Shield Strikes";
        break;

    case BossActions::ShieldThrow:
        return "Shield Throw";
        break;

    case BossActions::WaterSpouts:
        return "WaterSpouts";
        break;

    case BossActions::OverheadStrike:
        return "OverheadStrike";
        break;

    case BossActions::Mirage:
        return "Mirage";
        break;

    default:
        return "ERROR: NO ACTION";
        break;
    }
}