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
#include "ScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

Boss::Boss(GameObject* parent) : Character(parent, 60, 1, 0.5f, 1.0f, 1.0f, 3.0f, 15.0f, 20.0f, CharacterType::Boss)
{
    fields.push_back({"Dash Duration", InspectorField::FieldType::Float, &dashDuration, 0.0f, 2.0f});
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

    GameObject* arenaGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("arena");
    if (arenaGO)
    {
        ScriptComponent* sc = arenaGO->GetComponent<ScriptComponent*>();
        if (sc && sc->GetScriptByType<BossMirage>())
        {
            bossMirageScript = sc->GetScriptByType<BossMirage>();
        }
    }
    else
    {
        GLOG("Boss arena not found");
    }

    return true;
}

void Boss::Update(float deltaTime)
{
    if (agentAI == nullptr || isDead) return;
    Character::Update(deltaTime);

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life        = "Health: " + std::to_string(currentHealth);
        const std::string animState   = "Anim state: " + stateName.GetString();
        const std::string logicAction = "Action: " + std::string(GetActionName());
        const std::string logicState  = "State: " + std::string(GetStateName());
        const std::string phaseState  = "Phase: " + std::to_string(phase);

        std::vector<std::pair<std::string, float2>> logs {
            {life,        float2(-50.0f, -140.0f)},
            {animState,   float2(-80.0f, -160.0f)},
            {logicAction, float2(-80.0f, -180.0f)},
            {logicState,  float2(-80.0f, -200.0f)},
            {phaseState,  float2(-50.0f, -220.0f)},
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

    // update healthbar
    // TODO: play boss take damage sound
    // TODO: particles? and animation
}

void Boss::HandleState(float deltaTime)
{
    if (!mirageActivated && currentHealth <= mirageActivation[phase - 1]) currentState = BossStates::Mirage;

    if (currentHealth <= phaseSwap[phase - 1])
    {
        phase++;
        currentState = BossStates::ChangePhase;
    }

    switch (currentState)
    {
    case BossStates::Idle:
        Idle();
        break;

    case BossStates::Taunt:
        Taunt(deltaTime);
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

    case BossStates::ChangePhase:
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
    switch (phase)
    {
    case 1:
        ChooseNextStateFirstPhase();
        break;
    case 2:
        ChooseNextStateSecondPhase();
        break;

    case 3:
        ChooseNextStateThirdPhase();
        break;

    default:
        GLOG("Invalid phase boss")
        break;
    }
}

void Boss::ChooseNextStateFirstPhase()
{
    int shieldStrikesRate  = 0;
    int overheadStrikeRate = 0;

    // Set states ratio depending on distance
    switch (CheckDistanceWithPlayer())
    {
    case PlayerDistances::Close:
        // overheadStrikeRate = 100;
        shieldStrikesRate  = 75;
        overheadStrikeRate = 100;
        break;

    case PlayerDistances::Medium:
        // overheadStrikeRate = 100;
        shieldStrikesRate  = 50;
        overheadStrikeRate = 100;
        break;

    case PlayerDistances::Far:
        float distance = character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
        if (distance <= maxDetectionRange) doTaunt = true;
        else doIdle = true;
        break;
    }

    int num = uniformDist(rng);
    if (doTaunt)
    {
        currentState = BossStates::Taunt;
    }
    else if (doIdle)
    {
        currentState = BossStates::Idle;
    }
    else
    {
        if (num <= shieldStrikesRate)
        {
            currentState = BossStates::ShieldStrikes;
        }
        else if (num <= overheadStrikeRate)
        {
            currentState = BossStates::OverheadStrike;
        }
    }

    stateEnter = true;
}

void Boss::ChooseNextStateSecondPhase()
{
}

void Boss::ChooseNextStateThirdPhase()
{
}

void Boss::Idle()
{
    if (stateEnter)
    {

        // TODO: Randomize the idle duration
        // agentAI->SetSpeed(0.0f, 10.0f);

        stateEnter    = false;
        currentAction = BossActions::Idle;
        doIdle        = false;

        if (animComponent) animComponent->UseTrigger("Idle");
    }

    ChooseNextState();
}

void Boss::Taunt(float deltaTime)
{
    if (stateEnter)
    {

        stateEnter    = false;
        doTaunt       = false;
        currentAction = BossActions::Taunt;

        if (animComponent) animComponent->UseTrigger("Taunt");
    }
    agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent && animComponent->IsFinished()) Idle();
    else ChooseNextState();
}

void Boss::ShieldStrikes(float deltaTime)
{
    if (!weaponCollider) return;

    if (stateEnter)
    {

        stateEnter             = false;
        currentAction          = BossActions::Chase;
        shieldStrikeLastAction = 0;
    }

    switch (currentAction)
    {
    case BossActions::Chase:
        if (!actionTriggerDone)
        {
            animComponent->UseTrigger("Run");
            actionTriggerDone = true;
        }

        agentAI->SetPathNavigation(character->GetLastPosition());

        if (CheckDistanceWithPlayer() == PlayerDistances::Close)
        {
            if (shieldStrikeLastAction == 1) currentAction = BossActions::Combo2;
            else if (shieldStrikeLastAction == 2) currentAction = BossActions::Combo3;
            else currentAction = BossActions::Combo1;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Combo1:
        if (!actionTriggerDone)
        {
            attackHitboxDelay    = 1.0f;
            attackHitboxDuration = 0.3f;
            Character::Attack(deltaTime);
            agentAI->PauseMovement();
            if (animComponent) animComponent->UseTrigger("Combo1");
            actionTriggerDone      = true;
            shieldStrikeLastAction = 1;
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        if (animComponent && animComponent->IsFinished())
        {
            if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentAction = BossActions::Combo2;
            else currentAction = BossActions::Chase;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Combo2:
        if (!actionTriggerDone)
        {
            attackHitboxDelay    = 0.7f;
            attackHitboxDuration = 0.3f;
            Character::Attack(deltaTime);
            agentAI->PauseMovement();
            if (animComponent) animComponent->UseTrigger("Combo2");
            actionTriggerDone      = true;
            shieldStrikeLastAction = 2;
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        if (animComponent && animComponent->IsFinished())
        {
            if (CheckDistanceWithPlayer() == PlayerDistances::Close) currentAction = BossActions::Combo3;
            else currentAction = BossActions::Chase;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Combo3:
        if (!actionTriggerDone)
        {
            attackHitboxDelay    = 0.9f;
            attackHitboxDuration = 0.9f;
            Character::Attack(deltaTime);
            agentAI->PauseMovement();
            if (animComponent) animComponent->UseTrigger("Combo3");
            actionTriggerDone      = true;
            shieldStrikeLastAction = 3;
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        if (animComponent && animComponent->IsFinished())
        {
            actionTriggerDone = false;
            isAttacking       = false;
            attackCdTimer     = attackCooldown;
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
        agentAI->PauseMovement();
    }
    else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
    {
        weaponCollider->SetEnabled(false);
        agentAI->ResumeMovement();
    }

    agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
}

void Boss::OverheadStrike(float deltaTime)
{
    if (!weaponCollider) return;

    if (stateEnter)
    {

        stateEnter    = false;
        currentAction = BossActions::Prepare;
    }

    switch (currentAction)
    {
    case BossActions::Prepare:
        if (!actionTriggerDone)
        {
            if (animComponent) animComponent->UseTrigger("Prepare");
            actionTriggerDone = true;
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Jump;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Jump:
    {
        if (!actionTriggerDone)
        {
            agentAI->SetFreeMove(true);
            StartJump();
            if (animComponent) animComponent->UseTrigger("Jump");
            actionTriggerDone = true;
        }

        if (isJumping) Jump(deltaTime);
        else
        {
            GLOG("Finished jump")
            currentAction     = BossActions::Dash;
            actionTriggerDone = false;
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Dash;
            actionTriggerDone = false;
        }
        break;
    }

    case BossActions::Dash:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            StartDash();
            if (animComponent) animComponent->UseTrigger("Dash");
        }

        if (isDashing) Dash(deltaTime);
        else
        {
            GLOG("Finished dashing")
            currentAction     = BossActions::Land;
            actionTriggerDone = false;
        }

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Land;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Land:
        if (!actionTriggerDone)
        {
            StartJump();
            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Land");
        }

        if (isJumping) Jump(deltaTime, true);
        else
        {
            currentAction     = BossActions::Attack;
            actionTriggerDone = false;
            GLOG("Finished fall");
        }

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Attack;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Attack:
        if (!actionTriggerDone)
        {
            agentAI->SetFreeMove(false);
            attackHitboxDelay    = 0.5f;
            attackHitboxDuration = 0.2f;
            Character::Attack(deltaTime);
            agentAI->PauseMovement();
            if (animComponent) animComponent->UseTrigger("Attack");
            actionTriggerDone = true;
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        // Enable hitbox when animation hits
        if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
            attackTimer <= attackHitboxDelay + attackHitboxDuration)
        {
            weaponCollider->SetEnabled(true);
            agentAI->PauseMovement();
        }
        else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
        {
            weaponCollider->SetEnabled(false);
            agentAI->ResumeMovement();
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            isAttacking       = false;
            attackCdTimer     = attackCooldown;
            actionTriggerDone = false;
            currentAction     = BossActions::Recover;
        }
        break;

    case BossActions::Recover:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Recover");
        }

        if (animComponent && animComponent->IsFinished())
        {
            agentAI->ResumeMovement();
            actionTriggerDone = false;

            ChooseNextState();
        }
        break;

    default:
        GLOG("Error: OverheadStrike")
        break;
    }
}

void Boss::StartDash()
{
    isDashing        = true;

    float3 bossPos   = parent->GetGlobalTransform().TranslatePart();
    float3 playerPos = character->GetLastPosition();

    bossPos.y        = 0.0f;
    playerPos.y      = 0.0f;

    dashDistance     = (playerPos - bossPos).Length();
    dashDirection    = (playerPos - bossPos).Normalized();

    GLOG("Distance: %.2f", dashDistance);
    GLOG("Direction: %.2f %.2f %.2f", dashDirection.x, dashDirection.y, dashDirection.z);

    dashSpeed         = dashDistance / dashDuration;
    dashTimeRemaining = dashDuration;

    startPosLocal     = parent->GetLocalTransform().TranslatePart();

    GLOG("Speed: %.2f", dashSpeed);
}

void Boss::Dash(float deltaTime)
{
    dashTimeRemaining -= deltaTime;
    if (dashTimeRemaining < 0.0f) dashTimeRemaining = 0.0f;

    float elapsedTime       = dashDuration - dashTimeRemaining;
    float offsetDist        = dashSpeed * elapsedTime;

    float3 horizontalOffset = dashDirection * offsetDist;
    float originalY         = startPosLocal.y;

    float3 newPos           = startPosLocal + float3(horizontalOffset.x, 0.0f, horizontalOffset.z);
    newPos.y                = originalY;

    parent->SetLocalPosition(newPos);

    if (dashTimeRemaining <= 0.0f)
    {
        isDashing = false;
        parent->SetLocalPosition(startPosLocal + float3(horizontalOffset.x, 0.0f, horizontalOffset.z));
    }
}

void Boss::StartJump(bool fall)
{
    isJumping         = true;
    float duration    = fall ? fallDuration : jumpDuration;
    jumpSpeed         = heightJump / duration;
    jumpTimeRemaining = duration;
    startPosLocal     = parent->GetLocalTransform().TranslatePart();
    GLOG("Speed: %.2f", jumpSpeed);
}

void Boss::Jump(float deltaTime, bool fall)
{
    jumpTimeRemaining -= deltaTime;
    if (jumpTimeRemaining < 0.0f) jumpTimeRemaining = 0.0f;

    float elapsedTime = jumpDuration - jumpTimeRemaining;
    float offsetY     = jumpSpeed * elapsedTime;

    float3 dir        = fall ? -float3::unitY : float3::unitY;

    float3 newPos     = startPosLocal + dir * offsetY;

    parent->SetLocalPosition(newPos);

    if (jumpTimeRemaining <= 0.0f)
    {
        isJumping = false;
        parent->SetLocalPosition(startPosLocal + float3::unitY * heightJump);
    }
}

void Boss::Mirage()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Mirage");

        mirageActivated = true;
        stateEnter      = false;
        agentAI->PauseMovement();
        currentAction  = BossActions::Mirage;
        isInvulnerable = true;
        bossMirageScript->StartSequence(phase);
    }

    if ((int)bossMirageScript->GetSequenceState() == 0)
    {
        isInvulnerable = false;
        ChooseNextState();
    }
}

const char* Boss::GetStateName() const
{
    switch (currentState)
    {
    case BossStates::None:
        return "None";

    case BossStates::ChangePhase:
        return "ChangePhase";

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

    case BossActions::Chase:
        return "Chase";

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

    case BossActions::ChangeStart:
        return "ChangeStart";

    case BossActions::ChangeCharge:
        return "ChangeCharge";

    case BossActions::WaterSpouts:
        return "WaterSpouts";

    default:
        return "ERROR: NO ACTION";
    }
}