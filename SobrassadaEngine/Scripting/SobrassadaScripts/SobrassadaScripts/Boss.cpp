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
    fields.push_back({InspectorField::FieldType::Text, (void*)"Ferdiad specific"});
    fields.push_back({"Dash Duration", InspectorField::FieldType::Float, &dashDuration, 0.0f, 2.0f});
    fields.push_back({"Height Jump", InspectorField::FieldType::Float, &heightJump, 0.0f, 5.0f});
    fields.push_back({"Jump Duration", InspectorField::FieldType::Float, &jumpDuration, 0.0f, 2.0f});
    fields.push_back({"Fall Duration", InspectorField::FieldType::Float, &fallDuration, 0.0f, 2.0f});
    fields.push_back({"Close Area Damage", InspectorField::FieldType::Int, &closeAreaDamage, 0, 5});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Colliders"});
    fields.push_back({"Shield Collider", InspectorField::FieldType::InputText, &shieldName});
    fields.push_back({"Close Area", InspectorField::FieldType::InputText, &closeAreaName});
    fields.push_back({"Big Area", InspectorField::FieldType::InputText, &bigAreaName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"VFX"});
    fields.push_back({"Dash", InspectorField::FieldType::InputText, &dashVFXName});
    fields.push_back({"Area Overhead", InspectorField::FieldType::InputText, &areaOverheadVFXName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Particle"});
    fields.push_back({"Atom", InspectorField::FieldType::InputText, &atomParticleName});
    fields.push_back({"Smoke", InspectorField::FieldType::InputText, &smokeParticleName});
    fields.push_back({"Charge Shield", InspectorField::FieldType::InputText, &chargeShieldParticleName});
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

    rng                      = std::mt19937(std::random_device {}());
    uniformDist              = std::uniform_int_distribution<int>(0, 100);

    GameObject* shieldObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(shieldName);
    if (shieldObject)
    {
        weaponCollider = shieldObject->GetComponent<CapsuleColliderComponent*>();
        if (weaponCollider) weaponCollider->SetEnabled(false);
        else GLOG("Ferdiad without shield collider");
    }

    closeArea = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(closeAreaName);
    if (closeArea) closeArea->SetEnabled(false);
    else GLOG("Not close area object found for ferdiad");

    bigArea = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(bigAreaName);
    if (bigArea) bigArea->SetEnabled(false);
    else GLOG("Not big area object found for ferdiad");

    dashVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(dashVFXName);
    if (dashVFX) dashVFX->SetEnabled(false);
    else GLOG("Dash VFX not found for ferdiad");

    areaOverheadVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(areaOverheadVFXName);
    if (areaOverheadVFX) areaOverheadVFX->SetEnabled(false);
    else GLOG("Area overhead VFX not found for ferdiad");

    atomParticle = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(atomParticleName);
    if (atomParticle) atomParticle->SetEnabled(false);
    else GLOG("Atom particle not found for ferdiad");

    smokeParticle = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(smokeParticleName);
    if (smokeParticle) smokeParticle->SetEnabled(false);
    else GLOG("Smoke particle not found for ferdiad");

    chargeShieldParticle = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(chargeShieldParticleName);
    if (chargeShieldParticle) chargeShieldParticle->SetEnabled(false);
    else GLOG("Smoke particle not found for ferdiad");

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
    if (!mirageActivated && currentHealth <= mirageActivation[phase - 1])
    {
        stateEnter   = true;
        currentState = BossStates::Mirage;
    }

    if (currentHealth <= phaseSwap[phase - 1])
    {
        stateEnter   = true;
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
        ChangePhase();
        break;

    case BossStates::WaterSpouts:
        break;
    }
}

void Boss::UpdateTimers(float deltaTime)
{
    Character::UpdateTimers(deltaTime);

    if (currentState == BossStates::Mirage) isInvulnerable = true;
}

void Boss::ChooseNextState()
{
    stateEnter = true;

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

// Phase1: ShieldStrikes, OverheadStrike & Mirage
void Boss::ChooseNextStateFirstPhase()
{
    int shieldStrikesRate  = -1;
    int overheadStrikeRate = -1;

    // Set states ratio depending on distance
    switch (CheckDistance())
    {
    case BossDistance::Close:
        shieldStrikesRate = 100;
        break;

    case BossDistance::Near:
        shieldStrikesRate  = 80;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate  = 60;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate  = 50;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Far:
        shieldStrikesRate  = 35;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate  = 20;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Extreme:
        float distance = character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
        if (distance <= maxDetectionRange) doTaunt = true;
        else doIdle = true;
        break;
    }
    shieldStrikesRate = -1;

    int num           = uniformDist(rng);
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
}

// Phase2: ShieldStrikes, ShieldBlast, Mirage & WaterSpouts
void Boss::ChooseNextStateSecondPhase()
{
    int shieldStrikesRate = -1;
    int shieldBlastRate   = -1;
    int waterSpoutsRate   = -1;

    // Set states ratio depending on distance
    switch (CheckDistance())
    {
    case BossDistance::Close:
        shieldStrikesRate = 100;
        break;

    case BossDistance::Near:
        shieldStrikesRate = 80;
        shieldBlastRate   = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate = 60;
        shieldBlastRate   = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate = 50;
        shieldBlastRate   = 100;
        break;

    case BossDistance::Far:
        shieldStrikesRate = 35;
        shieldBlastRate   = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate = 20;
        shieldBlastRate   = 100;
        break;

    case BossDistance::Extreme:
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
        else if (num <= shieldBlastRate)
        {
            currentState = BossStates::OverheadStrike;
        }
    }
}

// Phase3: (ALL) ShieldStrikes, OverheadStrike, ShieldBlast, Mirage & WaterSpouts
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
        ResetValues(true);
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
        actionTriggerDone      = false;
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
            StopAttacking();
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
            StopAttacking();
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
            StopAttacking();
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
    if (!closeArea || !bigArea) return; // TODO: vfx

    if (stateEnter)
    {
        stateEnter        = false;
        actionTriggerDone = false;
        currentAction     = BossActions::Prepare;
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
            if (animComponent) animComponent->UseTrigger("Jump");
            actionTriggerDone = true;
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
    {
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Dash");
            dashVFX->SetEnabled(true);
            StartDash();
            Attack(deltaTime);
            weaponCollider->SetEnabled(true);
        }

        bool finished = false;

        if (isDashing) Dash(deltaTime);
        else finished = true;

        if (animComponent && animComponent->IsFinished()) finished = true;

        if (finished)
        {
            weaponCollider->SetEnabled(false);
            StopAttacking();
            dashVFX->SetEnabled(false);
            currentAction     = BossActions::Land;
            actionTriggerDone = false;
        }
        break;
    }

    case BossActions::Land:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Land");
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
            actionTriggerDone = true;

            if (animComponent) animComponent->UseTrigger("Attack");

            agentAI->SetFreeMove(false);
            agentAI->PauseMovement();

            //AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
            //areaOverheadVFX->GetChildren().at(0);

            atomParticle->SetEnabled(true);
            smokeParticle->SetEnabled(true);

            attackHitboxDelay    = 0.7f;
            attackHitboxDuration = 2.0f;
            Character::Attack(deltaTime);
        }

        DamageAreaLogic();

        if (animComponent && animComponent->IsFinished())
        {
            actionTriggerDone = false;
            currentAction     = BossActions::Recover;
        }
        break;

    case BossActions::Recover:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;

            if (animComponent) animComponent->UseTrigger("Recover");

            chargeShieldParticle->SetEnabled(false);
        }

        DamageAreaLogic();

        if (animComponent && animComponent->IsFinished())
        {
            actionTriggerDone = false;
            currentAction     = BossActions::Waiting;
        }
        break;

    case BossActions::Waiting:
        if (animComponent && animComponent->IsFinished()) animComponent->UseTrigger("Idle");

        DamageAreaLogic();
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

    dashStartPosLocal = parent->GetLocalTransform().TranslatePart();

    GLOG("Speed: %.2f", dashSpeed);
}

void Boss::Dash(float deltaTime)
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

void Boss::StartJump()
{
    isJumping         = true;
    jumpSpeed         = heightJump / jumpDuration;
    jumpTimeRemaining = jumpDuration;
    jumpStartPosLocal = parent->GetLocalTransform().TranslatePart();
}

void Boss::Jump(float deltaTime)
{
    jumpTimeRemaining -= deltaTime;
    if (jumpTimeRemaining < 0.0f) jumpTimeRemaining = 0.0f;

    float elapsedTime = jumpDuration - jumpTimeRemaining;
    float offsetY     = jumpSpeed * elapsedTime;

    float3 newPos     = jumpStartPosLocal + float3::unitY * offsetY;

    parent->SetLocalPosition(newPos);

    if (jumpTimeRemaining <= 0.0f)
    {
        isJumping = false;
        parent->SetLocalPosition(jumpStartPosLocal + float3::unitY * heightJump);
    }
}

void Boss::StartFall()
{
    isFalling         = true;
    fallSpeed         = heightJump / fallDuration;
    fallTimeRemaining = fallDuration;
    fallStartPosLocal = parent->GetLocalTransform().TranslatePart();
    GLOG("Speed: %.2f", fallSpeed);
}

void Boss::Fall(float deltaTime)
{
    fallTimeRemaining -= deltaTime;
    if (fallTimeRemaining < 0.0f) fallTimeRemaining = 0.0f;

    float elapsedTime = fallDuration - fallTimeRemaining;
    float offsetY     = fallSpeed * elapsedTime;

    float3 newPos     = fallStartPosLocal - float3::unitY * offsetY;

    parent->SetLocalPosition(newPos);

    if (fallTimeRemaining <= 0.0f)
    {
        isFalling = false;
        parent->SetLocalPosition(fallStartPosLocal - float3::unitY * heightJump);
    }
}

void Boss::DamageAreaLogic()
{
    // Enable hitbox when animation hits
    if (!closeArea->IsEnabled() && attackTimer >= attackHitboxDelay &&
        attackTimer <= attackHitboxDelay + attackHitboxDuration)
    {
        closeArea->SetEnabled(true);

        atomParticle->SetEnabled(false);
        smokeParticle->SetEnabled(false);
        chargeShieldParticle->SetEnabled(true);
    }
    else if (closeArea->IsEnabled() && bigArea->IsEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
    {
        closeArea->SetEnabled(false);
        bigArea->SetEnabled(false);
        agentAI->ResumeMovement();
        StopAttacking();

        ChooseNextState();
    }

    if (!bigArea->IsEnabled() && attackTimer >= attackHitboxDelay &&
        attackTimer <= attackHitboxDelay + attackHitboxDuration && attackTimer >= bigAreaHitboxDelay)
    {
        bigArea->SetEnabled(true);
    }
}

BossDistance Boss::CheckDistance() const
{
    if (character != nullptr)
    {
        float distance = character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
        if (distance <= rangeAIAttack) return BossDistance::Close;
        else if (distance <= rangeAIChase / 3) return BossDistance::Near;
        else if (distance <= rangeAIChase / 2) return BossDistance::Medium;
        else if (distance <= rangeAIChase / 1.5f) return BossDistance::Distant;
        else if (distance <= rangeAIChase / 1.2f) return BossDistance::Far;
        else if (distance <= rangeAIChase) return BossDistance::Farther;
    }
    return BossDistance::Extreme;
}

void Boss::StopAttacking()
{
    isAttacking   = false;
    attackCdTimer = attackCooldown;
}

void Boss::Mirage()
{
    if (stateEnter)
    {
        GLOG("[BOSS] - Mirage");

        mirageActivated = true;
        stateEnter      = false;
        agentAI->PauseMovement();
        currentAction = BossActions::Start;
        bossMirageScript->StartSequence(phase);
    }

    switch (currentAction)
    {
    case BossActions::Start:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            animComponent->UseTrigger("Start");
        }

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Charge;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Charge:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            animComponent->UseTrigger("Charge");
        }

        if ((int)bossMirageScript->GetSequenceState() == 0)
        {
            GLOG("Leaving Mirage")
            currentAction     = BossActions::End;
            actionTriggerDone = false;
        }
        break;

    case BossActions::End:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            animComponent->UseTrigger("End");
        }

        if (animComponent && animComponent->IsFinished())
        {
            agentAI->ResumeMovement();
            actionTriggerDone = false;
            isInvulnerable    = false;

            ChooseNextState();
        }
        break;

    default:
        GLOG("Error: Mirage")
        break;
    }
}

void Boss::ResetValues(bool isForMirage)
{
    doIdle                 = false;
    doTaunt                = false;
    actionTriggerDone      = false;

    shieldStrikeLastAction = 0;

    isDashing              = false;
    dashSpeed              = 0.0f;
    dashTimeRemaining      = 0.0f;
    dashDistance           = 0.0f;
    dashDirection          = float3::zero;
    dashStartPosLocal      = float3::zero;

    isJumping              = false;
    jumpSpeed              = 0.0f;
    jumpTimeRemaining      = 0.0f;
    jumpStartPosLocal      = float3::zero;

    isFalling              = false;
    fallSpeed              = 0.0f;
    fallTimeRemaining      = 0.0f;
    fallStartPosLocal      = float3::zero;

    if (!isForMirage) mirageActivated = false;

    weaponCollider->SetEnabled(false);
    closeArea->SetEnabled(false);
    bigArea->SetEnabled(false);
}

void Boss::ShieldBlast(float deltaTime)
{
    if (!weaponCollider) return; // TODO: collider & vfx

    if (stateEnter)
    {
        stateEnter        = false;
        actionTriggerDone = false;
        currentAction     = BossActions::Load;
    }

    switch (currentAction)
    {
    case BossActions::Load:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            animComponent->UseTrigger("BlastCharge");
        }

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Shoot;
            actionTriggerDone = false;
        }
        break;

    case BossActions::Shoot:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            animComponent->UseTrigger("BlastHit");
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            actionTriggerDone = false;
            ChooseNextState();
        }
        break;

    default:
        GLOG("Error: ShieldBlast")
        break;
    }
}

void Boss::ChangePhase()
{
    if (stateEnter)
    {
        ResetValues(false);
        stateEnter = false;
        phase++;
        // TODO: anim changePhase
        currentAction = BossActions::Taunt;

        if (animComponent) animComponent->UseTrigger("Taunt");
    }

    if (animComponent && animComponent->IsFinished()) ChooseNextState();
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

    case BossActions::Waiting:
        return "Waiting";

    case BossActions::Start:
        return "Start";

    case BossActions::Charge:
        return "Charge";

    case BossActions::End:
        return "End";

    case BossActions::WaterSpouts:
        return "WaterSpouts";

    default:
        return "ERROR: NO ACTION";
    }
}