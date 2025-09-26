#include "pch.h"

#include "Application.h"
#include "AttackVfxSpritesheet.h"
#include "Boss.h"
#include "BossMirage.h"
#include "CameraComponent.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "Globals.h"
#include "MovingUVTransparent.h"
#include "ParticleSystemComponent.h"
#include "ResourceStateMachine.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Spouts.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

Boss::Boss(GameObject* parent) : Character(parent, 54, 1, 0.5f, 1.0f, 1.0f, 3.0f, 15.0f, 20.0f, CharacterType::Boss)
{
    fields.push_back({InspectorField::FieldType::Text, (void*)"Ferdiad specific"});
    fields.push_back({"Phase Start", InspectorField::FieldType::Int, &phase, 1, 3});
    fields.push_back({"1st Mirage", InspectorField::FieldType::Int, &mirage1, 0, 100});
    fields.push_back({"2nd Mirage", InspectorField::FieldType::Int, &mirage2, 0, 100});
    fields.push_back({"3rd Mirage", InspectorField::FieldType::Int, &mirage3, 0, 100});
    fields.push_back({"Dash Duration", InspectorField::FieldType::Float, &dashDuration, 0.0f, 2.0f});
    /*fields.push_back({"Height Jump", InspectorField::FieldType::Float, &heightJump, 0.0f, 5.0f});
    fields.push_back({"Jump Duration", InspectorField::FieldType::Float, &jumpDuration, 0.0f, 2.0f});
    fields.push_back({"Fall Duration", InspectorField::FieldType::Float, &fallDuration, 0.0f, 2.0f});*/
    fields.push_back({"Close Area Damage", InspectorField::FieldType::Int, &closeAreaDamage, 0, 5});
    fields.push_back({"Spout", InspectorField::FieldType::InputText, &spoutName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Colliders"});
    fields.push_back({"Shield Collider", InspectorField::FieldType::InputText, &shieldName});
    fields.push_back({"Overhead Close Area", InspectorField::FieldType::InputText, &closeAreaName});
    fields.push_back({"Overhead Big Area", InspectorField::FieldType::InputText, &bigAreaName});
    fields.push_back({"Blast Area", InspectorField::FieldType::InputText, &blastAreaName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"VFX"});
    fields.push_back({"Overhead Prepare", InspectorField::FieldType::InputText, &overheadPrepareVFXName});
    fields.push_back({"Overhead Dash", InspectorField::FieldType::InputText, &overheadDashVFXName});
    fields.push_back({"Overhead Attack", InspectorField::FieldType::InputText, &overheadAttackVFXName});
    fields.push_back({"Shield Blast", InspectorField::FieldType::InputText, &shieldBlastVFXName});

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

    // grab the 4 spouts in the arena
    for (int i = 1; i <= 4; ++i)
    {
        std::string spoutsNames = spoutName + std::to_string(i);
        GameObject* spout       = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(spoutsNames);
        if (!spout)
        {
            GLOG("Not spout game object found for ferdiad %s", spoutsNames.c_str());
            continue;
        }

        ScriptComponent* spoutScript = spout->GetComponent<ScriptComponent*>();
        if (!spoutScript)
        {
            GLOG("Not spout script component found for ferdiad");
            continue;
        }

        Spouts* spoutLogic = spoutScript->GetScriptByType<Spouts>();
        if (spoutLogic) waterSpouts.push_back(spoutLogic);
        else GLOG("Not spout script found for ferdiad");
    }

    GameObject* overheadPrepareVFX =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(overheadPrepareVFXName);
    if (overheadPrepareVFX)
    {
        GameObject* runesLightsObject = overheadPrepareVFX->GetChildGameObjectByName("Cyl_Charger_Lights");
        if (runesLightsObject)
        {
            MeshComponent* runesLightsMesh = runesLightsObject->GetComponent<MeshComponent*>();
            if (runesLightsMesh) runesLightsMesh->SetEnabled(false);
            else GLOG("Runes lights mesh not found for ferdiad");

            runesLightsScript = runesLightsObject->GetComponent<ShaderScriptComponent*>();
            if (runesLightsScript)
            {
                runesLightsScript->SetEnabled(false);

                runesLightsUV = runesLightsScript->GetScriptByType<MovingUVTransparent>();
                if (!runesLightsUV) GLOG("Runes lights script incorrect for ferdiad");
            }
            else GLOG("Runes lights script not found for ferdiad");
        }

        GameObject* runesObject = overheadPrepareVFX->GetChildGameObjectByName("Cyl_Charger");
        if (runesObject)
        {
            MeshComponent* runesMesh = runesObject->GetComponent<MeshComponent*>();
            if (runesMesh) runesMesh->SetEnabled(false);
            else GLOG("Runes mesh not found for ferdiad");

            runesScript = runesObject->GetComponent<ShaderScriptComponent*>();
            if (runesScript)
            {
                runesScript->SetEnabled(false);

                runesUV = runesScript->GetScriptByType<MovingUVTransparent>();
                if (!runesUV) GLOG("Runes script incorrect for ferdiad");
            }
            else GLOG("Runes shader script not found for ferdiad");
        }
        else GLOG("Runes VFX not found for ferdiad");
    }
    else GLOG("Overhead prepare VFX not found for ferdiad");

    GameObject* overheadDashVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(overheadDashVFXName);
    if (overheadDashVFX)
    {
        GameObject* dashGroundObject = overheadDashVFX->GetChildGameObjectByName("Dash_Energy_Ground");
        if (dashGroundObject)
        {
            dashGroundMesh = dashGroundObject->GetComponent<MeshComponent*>();
            if (dashGroundMesh) dashGroundMesh->SetEnabled(false);
            else GLOG("Dash ground mesh not found for ferdiad");
        }
        else GLOG("Dash ground VFX not found for ferdiad");

        GameObject* dashEnergyObject = overheadDashVFX->GetChildGameObjectByName("Dash_Energy");
        if (dashEnergyObject)
        {
            dashEnergyMesh = dashEnergyObject->GetComponent<MeshComponent*>();
            if (dashEnergyMesh) dashEnergyMesh->SetEnabled(false);
            else GLOG("Dash energy mesh not found for ferdiad");
        }
        else GLOG("Dash energy VFX not found for ferdiad");

        GameObject* dashLightsShieldObject = overheadDashVFX->GetChildGameObjectByName("Lights_Shield");
        if (dashLightsShieldObject)
        {
            MeshComponent* dashLightsShieldMesh = dashLightsShieldObject->GetComponent<MeshComponent*>();
            if (dashLightsShieldMesh) dashLightsShieldMesh->SetEnabled(false);
            else GLOG("Dash lights shield mesh not found for ferdiad");

            dashLightsShieldScript = dashLightsShieldObject->GetComponent<ShaderScriptComponent*>();
            if (dashLightsShieldScript)
            {
                dashLightsShieldScript->SetEnabled(false);

                dashLightsShieldUV = dashLightsShieldScript->GetScriptByType<MovingUVTransparent>();
                if (!dashLightsShieldUV) GLOG("Dash lights shield script incorrect for ferdiad");
            }
            else GLOG("Dash lights shield script not found for ferdiad");
        }
        else GLOG("Dash lights shield VFX not found for ferdiad");

        GameObject* dashShieldExpansion = overheadDashVFX->GetChildGameObjectByName("Shield_Expansion");
        if (dashShieldExpansion)
        {
            MeshComponent* dashShieldExpansionMesh = dashShieldExpansion->GetComponent<MeshComponent*>();
            if (dashShieldExpansionMesh) dashShieldExpansionMesh->SetEnabled(false);
            else GLOG("Dash shield expansion mesh not found for ferdiad");

            dashShieldExpansionScript = dashShieldExpansion->GetComponent<ShaderScriptComponent*>();
            if (dashShieldExpansionScript)
            {
                dashShieldExpansionScript->SetEnabled(false);

                dashShieldExpansionUV = dashShieldExpansionScript->GetScriptByType<MovingUVTransparent>();
                if (!dashShieldExpansionUV) GLOG("Dash shield expansion script incorrect for ferdiad");
            }
            else GLOG("Dash shield expansion script not found for ferdiad");
        }
        else GLOG("Dash lights shield VFX not found for ferdiad");
    }
    else GLOG("Overhead dash VFX not found for ferdiad");

    GameObject* overheadAttackVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(overheadAttackVFXName);
    if (overheadAttackVFX)
    {
        GameObject* attackExplosionObject = overheadAttackVFX->GetChildGameObjectByName("Cyl_Explosion");
        if (attackExplosionObject)
        {
            MeshComponent* attackExplosionMesh = attackExplosionObject->GetComponent<MeshComponent*>();
            if (attackExplosionMesh) attackExplosionMesh->SetEnabled(false);
            else GLOG("Attack explosion mesh not found for ferdiad");

            attackExplosionScript = attackExplosionObject->GetComponent<ShaderScriptComponent*>();
            if (attackExplosionScript)
            {
                attackExplosionScript->SetEnabled(false);

                attackExplosionUV = attackExplosionScript->GetScriptByType<MovingUVTransparent>();
                if (!attackExplosionUV) GLOG("Attack explosion script incorrect for ferdiad");
            }
            else GLOG("Attack explosion script not found for ferdiad");
        }
        else GLOG("Attack explosion VFX not found for ferdiad");

        GameObject* attackLightingsObject = overheadAttackVFX->GetChildGameObjectByName("Lightings");
        if (attackLightingsObject)
        {
            MeshComponent* attackLightingsMesh = attackLightingsObject->GetComponent<MeshComponent*>();
            if (attackLightingsMesh) attackLightingsMesh->SetEnabled(false);
            else GLOG("Attack lightings mesh not found for ferdiad");

            attackLightingsScript = attackLightingsObject->GetComponent<ShaderScriptComponent*>();
            if (attackLightingsScript)
            {
                attackLightingsScript->SetEnabled(false);

                attackLightingsUV = attackLightingsScript->GetScriptByType<MovingUVTransparent>();
                if (!attackLightingsUV) GLOG("Attack lightings script incorrect for ferdiad");
            }
            else GLOG("Attack lightings script not found for ferdiad");
        }
        else GLOG("Attack lightings VFX not found for ferdiad");

        GameObject* attackEnergyObject = overheadAttackVFX->GetChildGameObjectByName("Cyl_Energy");
        if (attackEnergyObject)
        {
            MeshComponent* attackEnergyMesh = attackEnergyObject->GetComponent<MeshComponent*>();
            if (attackEnergyMesh) attackEnergyMesh->SetEnabled(false);
            else GLOG("Attack energy mesh not found for ferdiad");

            attackEnergyScript = attackEnergyObject->GetComponent<ShaderScriptComponent*>();
            if (attackEnergyScript)
            {
                attackEnergyScript->SetEnabled(false);

                attackEnergyUV = attackEnergyScript->GetScriptByType<MovingUVTransparent>();
                if (!attackEnergyUV) GLOG("Attack energy script incorrect for ferdiad");
            }
            else GLOG("Attack energy script not found for ferdiad");
        }
        else GLOG("Attack energy VFX not found for ferdiad");

        GameObject* bigExpansionObject = overheadAttackVFX->GetChildGameObjectByName("Expansion_B");
        if (bigExpansionObject)
        {
            MeshComponent* bigExpansionMesh = bigExpansionObject->GetComponent<MeshComponent*>();
            if (bigExpansionMesh) bigExpansionMesh->SetEnabled(false);
            else GLOG("Big expansion mesh not found for ferdiad");

            bigExpansionScript = bigExpansionObject->GetComponent<ShaderScriptComponent*>();
            if (bigExpansionScript)
            {
                bigExpansionScript->SetEnabled(false);

                bigExpansionUV = bigExpansionScript->GetScriptByType<MovingUVTransparent>();
                if (!bigExpansionUV) GLOG("Big expansion script incorrect for ferdiad");
            }
            else GLOG("Big expansion script not found for ferdiad");
        }
        else GLOG("Big expansion VFX not found for ferdiad");

        GameObject* smallExpansionObject = overheadAttackVFX->GetChildGameObjectByName("Expansion_S");
        if (smallExpansionObject)
        {
            MeshComponent* smallExpansionMesh = smallExpansionObject->GetComponent<MeshComponent*>();
            if (smallExpansionMesh) smallExpansionMesh->SetEnabled(false);
            else GLOG("Small expansion mesh not found for ferdiad");

            smallExpansionScript = smallExpansionObject->GetComponent<ShaderScriptComponent*>();
            if (smallExpansionScript)
            {
                smallExpansionScript->SetEnabled(false);

                smallExpansionUV = smallExpansionScript->GetScriptByType<MovingUVTransparent>();
                if (!smallExpansionUV) GLOG("Small expansion script incorrect for ferdiad");
            }
            else GLOG("Small expansion script not found for ferdiad");
        }
        else GLOG("Small expansion VFX not found for ferdiad");
    }
    else GLOG("Overhead attack VFX not found for ferdiad");

    GameObject* shieldBlastVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(shieldBlastVFXName);
    if (shieldBlastVFX)
    {
        GameObject* blastPreHitObject = shieldBlastVFX->GetChildGameObjectByName("BlastShield_1");
        if (blastPreHitObject)
        {
            blastPreHitMesh = blastPreHitObject->GetComponent<MeshComponent*>();
            if (blastPreHitMesh) blastPreHitMesh->SetEnabled(false);
            else GLOG("Blast pre hit mesh not found for ferdiad");
        }
        else GLOG("Blast pre hit VFX not found for ferdiad");

        GameObject* blastSpriteSheetObject = shieldBlastVFX->GetChildGameObjectByName("BlastSprite");
        if (blastSpriteSheetObject)
        {
            blastSpriteSheetMesh = blastSpriteSheetObject->GetComponent<MeshComponent*>();
            if (blastSpriteSheetMesh) blastSpriteSheetMesh->SetEnabled(false);
            else GLOG("Blast sprite sheet mesh not found for ferdiad");

            blastSpriteScript = blastSpriteSheetObject->GetComponent<ShaderScriptComponent*>();
            if (blastSpriteScript)
            {
                blastSpriteScript->SetEnabled(false);

                blastSpritesheet = blastSpriteScript->GetScriptByType<AttackVfxSpritesheet>();
                if (!blastSpritesheet) GLOG("Blast sprite sheet script incorrect for ferdiad");
            }
            else GLOG("Blast sprite sheet script not found for ferdiad");
        }
        else GLOG("Blast sprite sheet hit object not found for ferdiad");

        GameObject* blastAreaObject = shieldBlastVFX->GetChildGameObjectByName(blastAreaName);
        if (blastAreaObject)
        {
            blastArea = blastAreaObject->GetComponent<CapsuleColliderComponent*>();
            if (blastArea) blastArea->SetEnabled(false);
            else GLOG("Not blast area capsule collider found for ferdiad");
        }
        else GLOG("Not blast area object found for ferdiad");
    }
    else GLOG("Shield blast VFX not found for ferdiad");

    GameObject* atomObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(atomParticleName);
    if (atomObject)
    {
        atomParticle = atomObject->GetComponent<ParticleSystemComponent*>();
        if (atomParticle) atomParticle->StopInstances();
        else GLOG("Particle component atom not found for ferdiad");
    }
    else GLOG("Atom particle object not found for ferdiad");

    GameObject* smokeObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(smokeParticleName);
    if (smokeObject)
    {
        smokeParticle = smokeObject->GetComponent<ParticleSystemComponent*>();
        if (smokeParticle) smokeParticle->StopInstances();
        else GLOG("Particle component smoke not found for ferdiad");
    }
    else GLOG("Smoke particle object not found for ferdiad");

    GameObject* chargeShieldObject =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(chargeShieldParticleName);
    if (chargeShieldObject)
    {
        chargeShieldParticle = chargeShieldObject->GetComponent<ParticleSystemComponent*>();
        if (chargeShieldParticle) chargeShieldParticle->StopInstances();
        else GLOG("Particle component charge shield not found for ferdiad");
    }
    else GLOG("Charge shield particle object not found for ferdiad");

    GameObject* arenaGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("arena");
    if (arenaGO)
    {
        ScriptComponent* sc = arenaGO->GetComponent<ScriptComponent*>();
        if (sc && sc->GetScriptByType<BossMirage>())
        {
            bossMirageScript = sc->GetScriptByType<BossMirage>();
        }
        else GLOG("Not mirage script component found for ferdiad")
    }
    else GLOG("Boss arena not found for mirage");

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

    if (playerScript && playerScript->GetState() == CharacterStates::DEATH) restart = true;
}

void Boss::OnPlayerExitLocation()
{
    GLOG("EXIT")
    waiting = true;
}

void Boss::OnPlayerEnterLocation()
{
    GLOG("ENTER")
    waiting = false;

    doTaunt = true;
    // ChooseNextState();
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
        Idle(deltaTime);
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

    case BossStates::ShieldBlast:
        ShieldBlast(deltaTime);
        break;

    case BossStates::WaterSpouts:
        WaterSpouts();
        break;

    case BossStates::Restart:
        Restart(deltaTime);
        break;

    default:
        GLOG("ERROR: Ferdiad HandleState")
        break;
    }
}

void Boss::UpdateTimers(float deltaTime)
{
    Character::UpdateTimers(deltaTime);

    if (currentState == BossStates::Mirage || currentState == BossStates::ChangePhase) isInvulnerable = true;
}

void Boss::ChooseNextState()
{
    stateEnter = true;

    if (restart)
    {
        currentState = BossStates::Restart;
        return;
    }

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
        shieldStrikesRate  = 30;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate  = 15;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Extreme:
        float distance = character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
        // if (distance <= maxDetectionRange) doTaunt = true;
        // else doIdle = true;
        // doIdle         = true;
        break;
    }

    int num = uniformDist(rng);
    if (doTaunt)
    {
        GLOG("DO TAUNT")
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
            SetState(BossStates::ShieldStrikes);
        }
        else if (num <= overheadStrikeRate)
        {
            SetState(BossStates::OverheadStrike);
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
        shieldStrikesRate = 40;
        shieldBlastRate   = 80;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate = 30;
        shieldBlastRate   = 60;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate = 30;
        shieldBlastRate   = 70;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Far:
        shieldStrikesRate = 20;
        shieldBlastRate   = 70;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate = 10;
        shieldBlastRate   = 80;
        waterSpoutsRate   = 100;
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
            SetState(BossStates::ShieldStrikes);
        }
        else if (num <= shieldBlastRate)
        {
            SetState(BossStates::ShieldBlast);
        }
        else if (num <= waterSpoutsRate)
        {
            SetState(BossStates::WaterSpouts);
        }
    }
}

// Phase3: (ALL) ShieldStrikes, OverheadStrike, ShieldBlast, Mirage & WaterSpouts
void Boss::ChooseNextStateThirdPhase()
{
    int shieldStrikesRate  = -1;
    int overheadStrikeRate = -1;
    int shieldBlastRate    = -1;
    int waterSpoutsRate    = -1;

    // Set states ratio depending on distance
    switch (CheckDistance())
    {
    case BossDistance::Close:
        shieldStrikesRate  = 55;
        overheadStrikeRate = 80;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Near:
        shieldStrikesRate  = 60;
        overheadStrikeRate = 70;
        shieldBlastRate    = 90;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate  = 30;
        overheadStrikeRate = 60;
        shieldBlastRate    = 80;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate  = 20;
        overheadStrikeRate = 55;
        shieldBlastRate    = 80;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Far:
        shieldBlastRate    = 10;
        overheadStrikeRate = 40;
        shieldBlastRate    = 80;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate = 10;
        shieldBlastRate   = 90;
        waterSpoutsRate   = 100;
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
            SetState(BossStates::ShieldStrikes);
        }
        else if (num <= overheadStrikeRate)
        {
            SetState(BossStates::OverheadStrike);
        }
        else if (num <= shieldBlastRate)
        {
            SetState(BossStates::ShieldBlast);
        }
        else if (num <= waterSpoutsRate)
        {
            SetState(BossStates::WaterSpouts);
        }
    }
}

void Boss::Idle(float deltaTime)
{
    if (stateEnter)
    {

        // TODO: Randomize the idle duration
        // agentAI->SetSpeed(0.0f, 10.0f);
        if (doIdle) ResetValues(false);
        stateEnter    = false;
        doIdle        = false;
        currentAction = BossActions::Idle;
        agentAI->PauseMovement();

        if (animComponent) animComponent->UseTrigger("Idle");
    }

    if (!waiting)
    {
        GLOG("CHOOSE NEXT STATE")
        ChooseNextState();
    }
    else
    {
        agentAI->ResumeMovement();
        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);
    }
}

void Boss::Taunt(float deltaTime)
{
    if (stateEnter)
    {
        if (doTaunt) ResetValues(false);
        stateEnter    = false;
        doTaunt       = false;
        currentAction = BossActions::Taunt;
        agentAI->PauseMovement();

        if (animComponent) animComponent->UseTrigger("Taunt");
    }
    agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent && animComponent->IsFinished()) Idle(deltaTime);
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
            agentAI->ResumeMovement();
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

        switch (CheckDistance()) // if far change mechanic
        {
        case BossDistance::Far:
        case BossDistance::Farther:
        case BossDistance::Extreme:
            agentAI->PauseMovement();
            ChooseNextState();
            break;
        default:
            break;
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
    if (!closeArea || !bigArea) ChooseNextState();

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
            agentAI->ResumeMovement();

            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Prepare");

            if (runesScript) runesScript->SetEnabled(true);
            if (runesLightsScript) runesLightsScript->SetEnabled(true);
        }
        else runesUV->SetPaused(true);

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Jump;
            actionTriggerDone = false;

            if (runesUV)
            {
                runesUV->SetPaused(false);
                runesUV->Reset();
            }
            if (runesLightsUV) runesLightsUV->Reset();
            if (runesScript) runesScript->SetEnabled(false);
            if (runesLightsScript) runesLightsScript->SetEnabled(false);
        }
        break;

    case BossActions::Jump:
    {
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            if (animComponent) animComponent->UseTrigger("Jump");

            agentAI->SetFreeMove(true);
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

            StartDash();
            Attack(deltaTime);
            if (weaponCollider) weaponCollider->SetEnabled(true);

            if (dashGroundMesh) dashGroundMesh->SetEnabled(true);
            if (dashEnergyMesh) dashEnergyMesh->SetEnabled(true);
            if (dashLightsShieldScript) dashLightsShieldScript->SetEnabled(true);
            if (dashShieldExpansionScript) dashShieldExpansionScript->SetEnabled(true);
        }

        bool finished = false;

        if (isDashing) Dash(deltaTime);
        else finished = true;

        if (animComponent && animComponent->IsFinished()) finished = true;

        if (finished)
        {
            currentAction     = BossActions::Land;
            actionTriggerDone = false;

            if (weaponCollider) weaponCollider->SetEnabled(false);
            StopAttacking();

            if (dashGroundMesh) dashGroundMesh->SetEnabled(false);
            if (dashEnergyMesh) dashEnergyMesh->SetEnabled(false);
            if (dashLightsShieldScript) dashLightsShieldScript->SetEnabled(false);
            if (dashShieldExpansionScript) dashShieldExpansionScript->SetEnabled(false);
            if (dashLightsShieldUV) dashLightsShieldUV->Reset();
            if (dashShieldExpansionUV) dashShieldExpansionUV->Reset();
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

            if (attackLightingsScript) attackLightingsScript->SetEnabled(true);
        }
        break;

    case BossActions::Attack:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;

            if (animComponent) animComponent->UseTrigger("Attack");

            agentAI->SetFreeMove(false);
            agentAI->PauseMovement();

            attackHitboxDelay    = 0.7f;
            attackHitboxDuration = 1.5f;
            Character::Attack(deltaTime);

            if (attackEnergyScript) attackEnergyScript->SetEnabled(true);

            if (atomParticle) atomParticle->Init();
            if (smokeParticle) smokeParticle->Init();
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
    // --- PRE HITTING GROUND ---
    if (attackTimer < attackHitboxDelay && attackEnergyScript->GetEnabled())
    {
        if (attackTimer >= 0.3f)
        {
            if (attackEnergyUV) attackEnergyUV->SetPaused(true);
        }

        if (attackTimer >= 0.4f)
        {
            if (atomParticle) atomParticle->StopInstances();
            if (smokeParticle) smokeParticle->StopInstances();

            if (attackEnergyScript) attackEnergyScript->SetEnabled(false);
            if (attackLightingsScript) attackLightingsScript->SetEnabled(false);
        }
    }

    if (attackTimer >= 0.6f && attackTimer <= attackHitboxDelay)
    {
        if (chargeShieldParticle) chargeShieldParticle->Init();
    }

    // --- IMPACT / HITBOX ACTIVE ---
    bool insideHitboxWindow =
        (attackTimer >= attackHitboxDelay && attackTimer <= attackHitboxDelay + attackHitboxDuration);

    if (insideHitboxWindow)
    {
        if (!closeArea->IsEnabled())
        {
            closeArea->SetEnabled(true);

            if (attackExplosionScript) attackExplosionScript->SetEnabled(true);
            if (smallExpansionScript) smallExpansionScript->SetEnabled(true);
        }

        if (attackTimer >= attackHitboxDelay + 0.2f)
        {
            if (attackExplosionScript) attackExplosionScript->SetEnabled(false);
        }

        if (!bigExpansionScript->GetEnabled() && attackTimer >= bigAreaHitboxDelay - 0.1f)
        {
            if (bigExpansionScript) bigExpansionScript->SetEnabled(true);
            if (chargeShieldParticle) chargeShieldParticle->StopInstances();
        }

        if (!bigArea->IsEnabled() && attackTimer >= bigAreaHitboxDelay)
        {
            bigArea->SetEnabled(true);

            if (smallExpansionScript) smallExpansionScript->SetEnabled(false);
        }
    }

    // --- END OF ATTACK ---
    if (closeArea->IsEnabled() && bigArea->IsEnabled() && attackTimer > attackHitboxDelay + attackHitboxDuration)
    {
        closeArea->SetEnabled(false);
        bigArea->SetEnabled(false);
        if (bigExpansionScript) bigExpansionScript->SetEnabled(false);

        agentAI->ResumeMovement();
        StopAttacking();

        if (attackEnergyUV)
        {
            attackEnergyUV->SetPaused(false);
            attackEnergyUV->Reset();
        }
        if (attackLightingsUV) attackLightingsUV->Reset();

        if (attackExplosionUV) attackExplosionUV->Reset();
        if (smallExpansionUV) smallExpansionUV->Reset();
        if (bigExpansionUV) bigExpansionUV->Reset();

        ChooseNextState();
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

        ResetValues(false);
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

void Boss::WaterSpouts()
{

    if (stateEnter)
    {
        stateEnter        = false;
        actionTriggerDone = false;
        currentAction     = BossActions::WaterSpoutCharge;
    }

    switch (currentAction)
    {
    case BossActions::WaterSpoutCharge:
        if (!actionTriggerDone)
        {
            agentAI->PauseMovement();
            if (animComponent) animComponent->UseTrigger("WaterSpoutCharge");
            actionTriggerDone = true;
        }

        if (animComponent && animComponent->IsFinished())
        {
            for (Spouts* spout : waterSpouts)
            {
                if (spout) spout->ForceActivate();
            }

            currentAction     = BossActions::WaterSpoutHit;
            actionTriggerDone = false;
        }
        break;

    case BossActions::WaterSpoutHit:
        if (!actionTriggerDone)
        {
            if (animComponent) animComponent->UseTrigger("WaterSpoutHit"); // spout hit animation
            actionTriggerDone = true;
        }

        if (animComponent && animComponent->IsFinished())
        {
            agentAI->ResumeMovement();
            actionTriggerDone = false;

            ChooseNextState(); // go back to AI loop
        }
        break;

    default:
        GLOG("Error: WaterSpouts");
        break;
    }
}

void Boss::ResetValues(bool changePhase)
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

    animComponent->OnResume();

    if (changePhase) mirageActivated = false;

    if (weaponCollider) weaponCollider->SetEnabled(false);
    if (closeArea) closeArea->SetEnabled(false);
    if (bigArea) bigArea->SetEnabled(false);

    if (runesScript) runesScript->SetEnabled(false);
    if (runesUV)
    {
        runesUV->SetPaused(false);
        runesUV->Reset();
    }
    if (runesLightsScript) runesLightsScript->SetEnabled(false);
    if (runesLightsUV) runesLightsUV->Reset();
    if (dashGroundMesh) dashGroundMesh->SetEnabled(false);
    if (dashEnergyMesh) dashEnergyMesh->SetEnabled(false);
    if (dashLightsShieldScript) dashLightsShieldScript->SetEnabled(false);
    if (dashLightsShieldUV) dashLightsShieldUV->Reset();
    if (dashShieldExpansionScript) dashShieldExpansionScript->SetEnabled(false);
    if (dashShieldExpansionUV) dashShieldExpansionUV->Reset();
    if (attackLightingsScript) attackLightingsScript->SetEnabled(false);
    if (attackLightingsUV) attackLightingsUV->Reset();
    if (attackEnergyScript) attackEnergyScript->SetEnabled(false);
    if (attackEnergyUV)
    {
        attackEnergyUV->SetPaused(false);
        attackEnergyUV->Reset();
    }
    if (attackExplosionScript) attackExplosionScript->SetEnabled(false);
    if (attackExplosionUV) attackExplosionUV->Reset();
    if (bigExpansionScript) bigExpansionScript->SetEnabled(false);
    if (bigExpansionUV) bigExpansionUV->Reset();
    if (smallExpansionScript) smallExpansionScript->SetEnabled(false);
    if (smallExpansionUV) smallExpansionUV->Reset();
    if (atomParticle) atomParticle->StopInstances();
    if (smokeParticle) smokeParticle->StopInstances();
    if (chargeShieldParticle) chargeShieldParticle->StopInstances();

    if (blastArea) blastArea->SetEnabled(false);
    if (blastPreHitMesh) blastPreHitMesh->SetEnabled(false);
    if (blastSpriteScript) blastSpriteScript->SetEnabled(false);

    agentAI->ResetAngularSpeed();
    agentAI->SetFreeMove(false);
}

void Boss::ShieldBlast(float deltaTime)
{
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
            agentAI->PauseMovement();
            actionTriggerDone = true;
            animComponent->UseTrigger("BlastCharge");
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            actionTriggerDone = false;
            currentAction     = BossActions::PreShoot;
        }
        break;

    case BossActions::PreShoot:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            animComponent->UseTrigger("BlastHit");

            attackHitboxDelay    = blastHitboxDelay;
            attackHitboxDuration = 2.0f;
            Character::Attack(deltaTime);
            agentAI->SetAngularSpeed(1.0f);
        }

        if (attackTimer >= 0.3f && blastPreHitMesh && !blastPreHitMesh->GetEnabled()) blastPreHitMesh->SetEnabled(true);

        if (attackTimer >= 0.5f) animComponent->OnPause();

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (attackTimer >= attackHitboxDelay)
        {
            actionTriggerDone = false;
            currentAction     = BossActions::Shoot;
        }
        break;

    case BossActions::Shoot:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;

            agentAI->SetAngularSpeed(0.5f);

            if (blastPreHitMesh) blastPreHitMesh->SetEnabled(false);
            if (blastArea) blastArea->SetEnabled(true);

            if (blastSpritesheet) blastSpritesheet->Reset();
            if (blastSpriteScript) blastSpriteScript->SetEnabled(true);
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (attackTimer >= attackHitboxDelay + attackHitboxDuration) animComponent->OnResume();

        if (animComponent && animComponent->IsFinished())
        {
            if (blastArea) blastArea->SetEnabled(false);

            if (blastSpriteScript) blastSpriteScript->SetEnabled(false);
            if (blastSpritesheet) blastSpritesheet->Reset();

            agentAI->ResetAngularSpeed();

            actionTriggerDone = false;
            ChooseNextState();
        }
        break;

    default:
        GLOG("Error: ShieldBlast")
        break;
    }
}

void Boss::SetState(BossStates newState)
{
    if (newState == currentState)
    {
        repeatedState++;
        if (repeatedState >= maxRepeats)
        {
            currentState  = ChooseAlternativeState();
            repeatedState = 0;
            return;
        }
    }
    else
    {
        repeatedState = 0;
    }

    currentState = newState;
}

BossStates Boss::ChooseAlternativeState() const
{
    std::vector<BossStates> allStates = GetAvailableStates();

    allStates.erase(std::remove(allStates.begin(), allStates.end(), currentState), allStates.end());

    int index = rand() % allStates.size();
    return allStates[index];
}

void Boss::Restart(float deltaTime)
{
    if (stateEnter)
    {
        restart           = false;
        stateEnter        = false;
        actionTriggerDone = false;

        currentAction     = BossActions::Return;
    }

    switch (currentAction)
    {
    case BossActions::Return:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;
            agentAI->ResumeMovement();
            animComponent->UseTrigger("Run");
        }

        agentAI->SetPathNavigation(startPos);

        if (CheckDistanceWithPoint(startPos))
        {
            GLOG("POINT REACHED");
            actionTriggerDone = false;
            doIdle            = true;
            waiting           = true;
            ChooseNextState();
        }
        break;

    default:
        GLOG("ERROR: Restart");
        break;
    }
}

void Boss::ChangePhase()
{
    if (stateEnter)
    {
        ResetValues(true);
        stateEnter = false;
        phase++;

        agentAI->PauseMovement();

        // TODO: anim changePhase
        currentAction = BossActions::Taunt;
        if (animComponent) animComponent->UseTrigger("Taunt");
    }

    if (animComponent && animComponent->IsFinished())
    {
        GLOG("ChangePhase Finished")
        agentAI->ResumeMovement();

        ChooseNextState();
    }
}

const std::vector<BossStates>& Boss::GetAvailableStates() const
{
    switch (phase)
    {
    case 1:
        return phase1States;
    case 2:
        return phase2States;
    case 3:
        return phase3States;
    default:
        break;
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

    case BossStates::ShieldBlast:
        return "ShieldBlast";

    case BossStates::Restart:
        return "Restart";

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

    case BossActions::WaterSpoutCharge:
        return "WaterSpoutCharge";

    case BossActions::WaterSpoutHit:
        return "WaterSpoutHit";

    case BossActions::Load:
        return "Load";

    case BossActions::PreShoot:
        return "PreShoot";

    case BossActions::Shoot:
        return "Shoot";

    case BossActions::Return:
        return "Return";

    default:
        return "ERROR: NO ACTION";
    }
}