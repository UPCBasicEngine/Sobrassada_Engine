#include "pch.h"

#include "Application.h"
#include "AttackVfxSpritesheet.h"
#include "BarFill.h"
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
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"

#include "Wwise_IDs.h"

Boss::Boss(GameObject* parent) : Character(parent, 54, 1, 0.5f, 1.0f, 1.0f, 3.0f, 15.0f, 20.0f, CharacterType::Boss)
{
    fields.push_back({InspectorField::FieldType::Text, (void*)"Ferdiad specific"});
    fields.push_back({"Change Scene", InspectorField::FieldType::InputText, &changeSceneName});
    fields.push_back({"Time to ChangeScene", InspectorField::FieldType::Float, &delayToChangeScene, 0.0f, 20.0f});
    fields.push_back({"Health Bar", InspectorField::FieldType::InputText, &healthBarName});
    fields.push_back({"Phase Start", InspectorField::FieldType::Int, &phase, 1, 3});
    fields.push_back({"Phase 2 Change", InspectorField::FieldType::Int, &phase2, 0, 100});
    fields.push_back({"Phase 3 Change", InspectorField::FieldType::Int, &phase3, 0, 100});
    fields.push_back({"1st Mirage", InspectorField::FieldType::Int, &mirage1, 0, 100});
    fields.push_back({"2nd Mirage", InspectorField::FieldType::Int, &mirage2, 0, 100});
    fields.push_back({"3rd Mirage", InspectorField::FieldType::Int, &mirage3, 0, 100});
    fields.push_back({"Dash Duration", InspectorField::FieldType::Float, &dashDuration, 0.0f, 2.0f});
    fields.push_back({"Step time", InspectorField::FieldType::Float, &stepTime, 0.0f, 1.0f});
    /*fields.push_back({"Height Jump", InspectorField::FieldType::Float, &heightJump, 0.0f, 5.0f});
    fields.push_back({"Jump Duration", InspectorField::FieldType::Float, &jumpDuration, 0.0f, 2.0f});
    fields.push_back({"Fall Duration", InspectorField::FieldType::Float, &fallDuration, 0.0f, 2.0f});*/
    fields.push_back({"Close Area Damage", InspectorField::FieldType::Int, &closeAreaDamage, 0, 5});
    fields.push_back({"Spout", InspectorField::FieldType::InputText, &spoutName});
    fields.push_back({"Highlight Delay", InspectorField::FieldType::Float, &highlightDelay, 0.0f, 10.0f});
    fields.push_back({"Chase Time Limit", InspectorField::FieldType::Float, &chaseTimeLimit, 0.0f, 20.0f});
    fields.push_back({"Blast Area Disabled", InspectorField::FieldType::Float, &blastAreaDisabledLimit, 0.0f, 5.0f});

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
    fields.push_back({"Emessive", InspectorField::FieldType::InputText, &emessiveVFXName});
    fields.push_back({"Invulnerable VFX", InspectorField::FieldType::InputText, &invulnerableVFXName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Particle"});
    fields.push_back({"Atom", InspectorField::FieldType::InputText, &atomParticleName});
    fields.push_back({"Smoke", InspectorField::FieldType::InputText, &smokeParticleName});
    fields.push_back({"Charge Shield", InspectorField::FieldType::InputText, &chargeShieldParticleName});
    fields.push_back({"Blast Energy", InspectorField::FieldType::InputText, &energyBlastParticleName});
}

bool Boss::Init()
{
    Character::Init();
    agentAI = parent->GetComponent<AIAgentComponent*>();
    if (agentAI == nullptr) GLOG("[WARNING] AIAgent component not found for Boss")
    else
    {
        agentAI->RecreateAgent();
        agentAI->SetLookForward(true);
        speed = agentAI->GetSpeed();
    }

    rng           = std::mt19937(std::random_device {}());
    uniformDist   = std::uniform_int_distribution<int>(0, 100);
    uniformSteps  = std::uniform_int_distribution<int>(1, 3);
    uniformGetHit = std::uniform_int_distribution<int>(1, 2);

    audio         = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] Ferdiad: No audio component found");

    changeScene = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(changeSceneName);
    if (changeScene) changeScene->SetEnabled(false);
    else GLOG("[WARNING] Ferdiad: Change scene object not found")

    GameObject* healthBarObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(healthBarName);
    if (healthBarObject)
    {
        healthBarBase = healthBarObject->GetComponent<ImageComponent*>();
        if (healthBarBase) healthBarBase->SetEnabled(false);
        else GLOG("[WARNING] Ferdiad: Health bar base image component not found");

        GameObject* healthBarFillObject = healthBarObject->GetChildGameObjectByName("BossHealthBarFill");
        if (healthBarFillObject)
        {
            healthBarShader = healthBarFillObject->GetComponent<ShaderScriptComponent*>();
            if (healthBarShader)
            {
                healthBarFill = healthBarShader->GetScriptByType<BarFill>();
                if (!healthBarFill) GLOG("[WARNING] Ferdiad: Health bar fill script component not found");
            }
            else GLOG("[WARNING] Ferdiad: Health bar shader component not found");
        }
        else GLOG("[WARNING] Ferdiad: Health bar fill object not found");

        GameObject* armorBarFillObject = healthBarObject->GetChildGameObjectByName("BossArmorBarFill");
        if (armorBarFillObject)
        {
            armorBarShader = armorBarFillObject->GetComponent<ShaderScriptComponent*>();
            if (armorBarShader)
            {
                armorBarFill = armorBarShader->GetScriptByType<BarFill>();
                if (!armorBarFill) GLOG("[WARNING] Ferdiad: Armor bar fill script component not found");
            }
            else GLOG("[WARNING] Ferdiad: Armor bar shader component not found");
        }
        else GLOG("[WARNING] Ferdiad: Armor bar fill object not found");
    }
    else GLOG("[WARNING] Ferdiad: Health bar base object not found");

    GameObject* shieldObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(shieldName);
    if (shieldObject)
    {
        weaponCollider = shieldObject->GetComponent<CapsuleColliderComponent*>();
        if (weaponCollider) weaponCollider->SetEnabled(false);
        else GLOG("[WARNING] Ferdiad without shield collider");
    }
    else GLOG("[WARNING] Ferdiad shield object by name not found");

    closeArea = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(closeAreaName);
    if (closeArea) closeArea->SetEnabled(false);
    else GLOG("[WARNING] Not close area object found for ferdiad");

    bigArea = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(bigAreaName);
    if (bigArea) bigArea->SetEnabled(false);
    else GLOG("[WARNING] Not big area object found for ferdiad");

    // grab the 4 spouts in the arena
    for (int i = 1; i <= 4; ++i)
    {
        std::string spoutsNames = spoutName + std::to_string(i);
        GameObject* spout       = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(spoutsNames);
        if (!spout)
        {
            GLOG("[WARNING] Not spout game object found for ferdiad %s", spoutsNames.c_str());
            continue;
        }

        ScriptComponent* spoutScript = spout->GetComponent<ScriptComponent*>();
        if (!spoutScript)
        {
            GLOG("[WARNING] Not spout script component found for ferdiad");
            continue;
        }

        Spouts* spoutLogic = spoutScript->GetScriptByType<Spouts>();
        if (spoutLogic) waterSpouts.push_back(spoutLogic);
        else GLOG("[WARNING] Not spout script found for ferdiad");
    }

    GameObject* emessiveVFXObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(emessiveVFXName);
    if (emessiveVFXObject)
    {
        emessiveVFXMesh = emessiveVFXObject->GetComponent<MeshComponent*>();
        if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(false);
        else GLOG("[WARNING] Not emessive mesh component found for ferdiad");
    }
    else GLOG("[WARNING] Not emessive VFX game object found for ferdiad");

    GameObject* overheadPrepareVFX =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(overheadPrepareVFXName);
    if (overheadPrepareVFX)
    {
        GameObject* runesLightsObject = overheadPrepareVFX->GetChildGameObjectByName("Cyl_Charger_Lights");
        if (runesLightsObject)
        {
            MeshComponent* runesLightsMesh = runesLightsObject->GetComponent<MeshComponent*>();
            if (runesLightsMesh) runesLightsMesh->SetEnabled(false);
            else GLOG("[WARNING] Runes lights mesh not found for ferdiad");

            runesLightsScript = runesLightsObject->GetComponent<ShaderScriptComponent*>();
            if (runesLightsScript)
            {
                runesLightsScript->SetEnabled(false);

                runesLightsUV = runesLightsScript->GetScriptByType<MovingUVTransparent>();
                if (!runesLightsUV) GLOG("[WARNING] Runes lights script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Runes lights script not found for ferdiad");
        }

        GameObject* runesObject = overheadPrepareVFX->GetChildGameObjectByName("Cyl_Charger");
        if (runesObject)
        {
            MeshComponent* runesMesh = runesObject->GetComponent<MeshComponent*>();
            if (runesMesh) runesMesh->SetEnabled(false);
            else GLOG("[WARNING] Runes mesh not found for ferdiad");

            runesScript = runesObject->GetComponent<ShaderScriptComponent*>();
            if (runesScript)
            {
                runesScript->SetEnabled(false);

                runesUV = runesScript->GetScriptByType<MovingUVTransparent>();
                if (!runesUV) GLOG("[WARNING] Runes script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Runes shader script not found for ferdiad");
        }
        else GLOG("[WARNING] Runes VFX not found for ferdiad");
    }
    else GLOG("[WARNING] Overhead prepare VFX not found for ferdiad");

    GameObject* overheadDashVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(overheadDashVFXName);
    if (overheadDashVFX)
    {
        GameObject* dashGroundObject = overheadDashVFX->GetChildGameObjectByName("Dash_Energy_Ground");
        if (dashGroundObject)
        {
            dashGroundMesh = dashGroundObject->GetComponent<MeshComponent*>();
            if (dashGroundMesh) dashGroundMesh->SetEnabled(false);
            else GLOG("[WARNING] Dash ground mesh not found for ferdiad");
        }
        else GLOG("[WARNING] Dash ground VFX not found for ferdiad");

        GameObject* dashEnergyObject = overheadDashVFX->GetChildGameObjectByName("Dash_Energy");
        if (dashEnergyObject)
        {
            dashEnergyMesh = dashEnergyObject->GetComponent<MeshComponent*>();
            if (dashEnergyMesh) dashEnergyMesh->SetEnabled(false);
            else GLOG("[WARNING] Dash energy mesh not found for ferdiad");
        }
        else GLOG("[WARNING] Dash energy VFX not found for ferdiad");

        GameObject* dashLightsShieldObject = overheadDashVFX->GetChildGameObjectByName("Lights_Shield");
        if (dashLightsShieldObject)
        {
            MeshComponent* dashLightsShieldMesh = dashLightsShieldObject->GetComponent<MeshComponent*>();
            if (dashLightsShieldMesh) dashLightsShieldMesh->SetEnabled(false);
            else GLOG("[WARNING] Dash lights shield mesh not found for ferdiad");

            dashLightsShieldScript = dashLightsShieldObject->GetComponent<ShaderScriptComponent*>();
            if (dashLightsShieldScript)
            {
                dashLightsShieldScript->SetEnabled(false);

                dashLightsShieldUV = dashLightsShieldScript->GetScriptByType<MovingUVTransparent>();
                if (!dashLightsShieldUV) GLOG("[WARNING] Dash lights shield script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Dash lights shield script not found for ferdiad");
        }
        else GLOG("[WARNING] Dash lights shield VFX not found for ferdiad");

        GameObject* dashShieldExpansion = overheadDashVFX->GetChildGameObjectByName("Shield_Expansion");
        if (dashShieldExpansion)
        {
            MeshComponent* dashShieldExpansionMesh = dashShieldExpansion->GetComponent<MeshComponent*>();
            if (dashShieldExpansionMesh) dashShieldExpansionMesh->SetEnabled(false);
            else GLOG("[WARNING] Dash shield expansion mesh not found for ferdiad");

            dashShieldExpansionScript = dashShieldExpansion->GetComponent<ShaderScriptComponent*>();
            if (dashShieldExpansionScript)
            {
                dashShieldExpansionScript->SetEnabled(false);

                dashShieldExpansionUV = dashShieldExpansionScript->GetScriptByType<MovingUVTransparent>();
                if (!dashShieldExpansionUV) GLOG("[WARNING] Dash shield expansion script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Dash shield expansion script not found for ferdiad");
        }
        else GLOG("[WARNING] Dash lights shield VFX not found for ferdiad");
    }
    else GLOG("[WARNING] Overhead dash VFX not found for ferdiad");

    GameObject* overheadAttackVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(overheadAttackVFXName);
    if (overheadAttackVFX)
    {
        GameObject* attackExplosionObject = overheadAttackVFX->GetChildGameObjectByName("Cyl_Explosion");
        if (attackExplosionObject)
        {
            MeshComponent* attackExplosionMesh = attackExplosionObject->GetComponent<MeshComponent*>();
            if (attackExplosionMesh) attackExplosionMesh->SetEnabled(false);
            else GLOG("[WARNING] Attack explosion mesh not found for ferdiad");

            attackExplosionScript = attackExplosionObject->GetComponent<ShaderScriptComponent*>();
            if (attackExplosionScript)
            {
                attackExplosionScript->SetEnabled(false);

                attackExplosionUV = attackExplosionScript->GetScriptByType<MovingUVTransparent>();
                if (!attackExplosionUV) GLOG("[WARNING] Attack explosion script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Attack explosion script not found for ferdiad");
        }
        else GLOG("[WARNING] Attack explosion VFX not found for ferdiad");

        GameObject* attackLightingsObject = overheadAttackVFX->GetChildGameObjectByName("Lightings");
        if (attackLightingsObject)
        {
            MeshComponent* attackLightingsMesh = attackLightingsObject->GetComponent<MeshComponent*>();
            if (attackLightingsMesh) attackLightingsMesh->SetEnabled(false);
            else GLOG("[WARNING] Attack lightings mesh not found for ferdiad");

            attackLightingsScript = attackLightingsObject->GetComponent<ShaderScriptComponent*>();
            if (attackLightingsScript)
            {
                attackLightingsScript->SetEnabled(false);

                attackLightingsUV = attackLightingsScript->GetScriptByType<MovingUVTransparent>();
                if (!attackLightingsUV) GLOG("[WARNING] Attack lightings script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Attack lightings script not found for ferdiad");
        }
        else GLOG("[WARNING] Attack lightings VFX not found for ferdiad");

        GameObject* attackEnergyObject = overheadAttackVFX->GetChildGameObjectByName("Cyl_Energy");
        if (attackEnergyObject)
        {
            MeshComponent* attackEnergyMesh = attackEnergyObject->GetComponent<MeshComponent*>();
            if (attackEnergyMesh) attackEnergyMesh->SetEnabled(false);
            else GLOG("[WARNING] Attack energy mesh not found for ferdiad");

            attackEnergyScript = attackEnergyObject->GetComponent<ShaderScriptComponent*>();
            if (attackEnergyScript)
            {
                attackEnergyScript->SetEnabled(false);

                attackEnergyUV = attackEnergyScript->GetScriptByType<MovingUVTransparent>();
                if (!attackEnergyUV) GLOG("[WARNING] Attack energy script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Attack energy script not found for ferdiad");
        }
        else GLOG("[WARNING] Attack energy VFX not found for ferdiad");

        GameObject* bigExpansionObject = overheadAttackVFX->GetChildGameObjectByName("Expansion_B");
        if (bigExpansionObject)
        {
            MeshComponent* bigExpansionMesh = bigExpansionObject->GetComponent<MeshComponent*>();
            if (bigExpansionMesh) bigExpansionMesh->SetEnabled(false);
            else GLOG("[WARNING] Big expansion mesh not found for ferdiad");

            bigExpansionScript = bigExpansionObject->GetComponent<ShaderScriptComponent*>();
            if (bigExpansionScript)
            {
                bigExpansionScript->SetEnabled(false);

                bigExpansionUV = bigExpansionScript->GetScriptByType<MovingUVTransparent>();
                if (!bigExpansionUV) GLOG("[WARNING] Big expansion script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Big expansion script not found for ferdiad");
        }
        else GLOG("[WARNING] Big expansion VFX not found for ferdiad");

        GameObject* smallExpansionObject = overheadAttackVFX->GetChildGameObjectByName("Expansion_S");
        if (smallExpansionObject)
        {
            MeshComponent* smallExpansionMesh = smallExpansionObject->GetComponent<MeshComponent*>();
            if (smallExpansionMesh) smallExpansionMesh->SetEnabled(false);
            else GLOG("[WARNING] Small expansion mesh not found for ferdiad");

            smallExpansionScript = smallExpansionObject->GetComponent<ShaderScriptComponent*>();
            if (smallExpansionScript)
            {
                smallExpansionScript->SetEnabled(false);

                smallExpansionUV = smallExpansionScript->GetScriptByType<MovingUVTransparent>();
                if (!smallExpansionUV) GLOG("[WARNING] Small expansion script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Small expansion script not found for ferdiad");
        }
        else GLOG("[WARNING] Small expansion VFX not found for ferdiad");
    }
    else GLOG("[WARNING] Overhead attack VFX not found for ferdiad");

    GameObject* shieldBlastVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(shieldBlastVFXName);
    if (shieldBlastVFX)
    {
        GameObject* blastPreHitObject = shieldBlastVFX->GetChildGameObjectByName("BlastSpritePre");
        if (blastPreHitObject)
        {
            MeshComponent* blastPreHitMesh = blastPreHitObject->GetComponent<MeshComponent*>();
            if (blastPreHitMesh) blastPreHitMesh->SetEnabled(false);
            else GLOG("[WARNING] Blast pre hit mesh not found for ferdiad");

            blastPreSpriteScript = blastPreHitObject->GetComponent<ShaderScriptComponent*>();
            if (blastPreSpriteScript)
            {
                blastPreSpriteScript->SetEnabled(false);

                blastPreSpritesheet = blastPreSpriteScript->GetScriptByType<AttackVfxSpritesheet>();
                if (!blastPreSpritesheet) GLOG("[WARNING] Blast pre sprite sheet script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Blast pre sprite sheet script not found for ferdiad");
        }
        else GLOG("[WARNING] Blast pre hit VFX not found for ferdiad");

        GameObject* blastSpriteSheetEnergyObject = shieldBlastVFX->GetChildGameObjectByName("BlastSpriteEnergy");
        if (blastSpriteSheetEnergyObject)
        {
            MeshComponent* blastSpriteSheetEnergyMesh = blastSpriteSheetEnergyObject->GetComponent<MeshComponent*>();
            if (blastSpriteSheetEnergyMesh) blastSpriteSheetEnergyMesh->SetEnabled(false);
            else GLOG("[WARNING] Blast sprite sheet energy mesh not found for ferdiad");

            blastEnergySpriteScript = blastSpriteSheetEnergyObject->GetComponent<ShaderScriptComponent*>();
            if (blastEnergySpriteScript)
            {
                blastEnergySpriteScript->SetEnabled(false);

                blastEnergySpritesheet = blastEnergySpriteScript->GetScriptByType<AttackVfxSpritesheet>();
                if (!blastEnergySpritesheet) GLOG("[WARNING] Blast sprite sheet energy script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Blast sprite sheet energy script not found for ferdiad");
        }
        else GLOG("[WARNING] Blast sprite sheet energy object not found for ferdiad");

        GameObject* blastSpriteSheetObject = shieldBlastVFX->GetChildGameObjectByName("BlastSprite");
        if (blastSpriteSheetObject)
        {
            MeshComponent* blastSpriteSheetMesh = blastSpriteSheetObject->GetComponent<MeshComponent*>();
            if (blastSpriteSheetMesh) blastSpriteSheetMesh->SetEnabled(false);
            else GLOG("[WARNING] Blast sprite sheet mesh not found for ferdiad");

            blastSpriteScript = blastSpriteSheetObject->GetComponent<ShaderScriptComponent*>();
            if (blastSpriteScript)
            {
                blastSpriteScript->SetEnabled(false);

                blastSpritesheet = blastSpriteScript->GetScriptByType<AttackVfxSpritesheet>();
                if (!blastSpritesheet) GLOG("[WARNING] Blast sprite sheet script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Blast sprite sheet script not found for ferdiad");
        }
        else GLOG("[WARNING] Blast sprite sheet hit object not found for ferdiad");

        GameObject* blastSpriteSheetObject2 = shieldBlastVFX->GetChildGameObjectByName("BlastSprite2");
        if (blastSpriteSheetObject2)
        {
            MeshComponent* blastSpriteSheetMesh2 = blastSpriteSheetObject2->GetComponent<MeshComponent*>();
            if (blastSpriteSheetMesh2) blastSpriteSheetMesh2->SetEnabled(false);
            else GLOG("[WARNING] Blast sprite sheet 2 mesh not found for ferdiad");

            blastSpriteScript2 = blastSpriteSheetObject2->GetComponent<ShaderScriptComponent*>();
            if (blastSpriteScript2)
            {
                blastSpriteScript2->SetEnabled(false);

                blastSpritesheet2 = blastSpriteScript2->GetScriptByType<AttackVfxSpritesheet>();
                if (!blastSpritesheet2) GLOG("[WARNING] Blast sprite sheet 2 script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Blast sprite sheet 2 script not found for ferdiad");
        }
        else GLOG("[WARNING] Blast sprite sheet 2 hit object not found for ferdiad");

        blastArea = shieldBlastVFX->GetChildGameObjectByName(blastAreaName);
        if (blastArea) blastArea->SetEnabled(false);
        else GLOG("[WARNING] Not blast area object found for ferdiad");
    }
    else GLOG("[WARNING] Shield blast VFX not found for ferdiad");

    GameObject* invulnerableVFX = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(invulnerableVFXName);
    if (invulnerableVFX)
    {
        GameObject* invulnerablePlaneWaterAnimationObject =
            invulnerableVFX->GetChildGameObjectByName("PlaneWaterAnimation");
        if (invulnerablePlaneWaterAnimationObject)
        {
            invulnerablePlaneWaterMesh = invulnerablePlaneWaterAnimationObject->GetComponent<MeshComponent*>();
            if (invulnerablePlaneWaterMesh) invulnerablePlaneWaterMesh->SetEnabled(false);
            else GLOG("[WARNING] Invulnerable plane water mesh not found for ferdiad");

            invulnerablePlaneWaterAnimation =
                invulnerablePlaneWaterAnimationObject->GetComponent<AnimationComponent*>();
            if (invulnerablePlaneWaterAnimation) invulnerablePlaneWaterAnimation->OnStop();
            else GLOG("[WARNING] Invulnerable plane water animation component not found for ferdiad");
        }
        else GLOG("[WARNING] Invulnerable plane water animation object not found for ferdiad");

        GameObject* energyBarrierObject = invulnerableVFX->GetChildGameObjectByName("EnergyBarrierVFX");
        if (energyBarrierObject)
        {
            MeshComponent* energyBarrierMesh = energyBarrierObject->GetComponent<MeshComponent*>();
            if (energyBarrierMesh) energyBarrierMesh->SetEnabled(false);
            else GLOG("[WARNING] Energy barrier mesh not found for ferdiad");

            invulnerableBarrierScript = energyBarrierObject->GetComponent<ShaderScriptComponent*>();
            if (invulnerableBarrierScript)
            {
                invulnerableBarrierScript->SetEnabled(false);

                invulnerableBarrierUV = invulnerableBarrierScript->GetScriptByType<MovingUVTransparent>();
                if (!invulnerableBarrierUV) GLOG("[WARNING] Invulnerable barrier script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Invulnerable barrier script not found for ferdiad");
        }
        else GLOG("[WARNING] Invulnerable barrier VFX object not found for ferdiad");

        GameObject* invulnerableShieldAnimationObject = invulnerableVFX->GetChildGameObjectByName("ShieldAnimation");
        if (invulnerableShieldAnimationObject)
        {
            invulnerableShieldMesh = invulnerableShieldAnimationObject->GetComponent<MeshComponent*>();
            if (invulnerableShieldMesh) invulnerableShieldMesh->SetEnabled(false);
            else GLOG("[WARNING] Invulnerable shield mesh not found for ferdiad");

            invulnerableShieldAnimation = invulnerableShieldAnimationObject->GetComponent<AnimationComponent*>();
            if (invulnerableShieldAnimation) invulnerableShieldAnimation->OnStop();
            else GLOG("[WARNING] Invulnerable shield animation component not found for ferdiad");
        }
        else GLOG("[WARNING] Invulnerable shield animation not found for ferdiad");

        GameObject* noisefallObject = invulnerableVFX->GetChildGameObjectByName("WaterNoisefallVFX");
        if (noisefallObject)
        {
            MeshComponent* noisefallMesh = noisefallObject->GetComponent<MeshComponent*>();
            if (noisefallMesh) noisefallMesh->SetEnabled(false);
            else GLOG("[WARNING] Invulnerable noisefall mesh not found for ferdiad");

            invulnerableNoisefallScript = noisefallObject->GetComponent<ShaderScriptComponent*>();
            if (invulnerableNoisefallScript)
            {
                invulnerableNoisefallScript->SetEnabled(false);

                invulnerableNoisefallUV = invulnerableNoisefallScript->GetScriptByType<MovingUVTransparent>();
                if (!invulnerableNoisefallUV) GLOG("[WARNING] Invulnerable noisefall script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Invulnerable noisefall script not found for ferdiad");
        }
        else GLOG("[WARNING] Invulnerable noisefall VFX object not found for ferdiad");

        GameObject* wavesObject = invulnerableVFX->GetChildGameObjectByName("WavesWaterVFX");
        if (wavesObject)
        {
            MeshComponent* wavesMesh = wavesObject->GetComponent<MeshComponent*>();
            if (wavesMesh) wavesMesh->SetEnabled(false);
            else GLOG("[WARNING] Invulnerable waves mesh not found for ferdiad");

            invulnerableWavesScript = wavesObject->GetComponent<ShaderScriptComponent*>();
            if (invulnerableWavesScript)
            {
                invulnerableWavesScript->SetEnabled(false);

                invulnerableWavesUV = invulnerableWavesScript->GetScriptByType<MovingUVTransparent>();
                if (!invulnerableWavesUV) GLOG("[WARNING] Invulnerable waves script incorrect for ferdiad");
            }
            else GLOG("[WARNING] Invulnerable waves script not found for ferdiad");
        }
        else GLOG("[WARNING] Invulnerable waves VFX object not found for ferdiad");
    }
    else GLOG("[WARNING] Invulnerable VFX game object not found for ferdiad");

    GameObject* atomObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(atomParticleName);
    if (atomObject)
    {
        atomParticle = atomObject->GetComponent<ParticleSystemComponent*>();
        if (atomParticle) atomParticle->StopInstances();
        else GLOG("[WARNING] Particle component atom not found for ferdiad");
    }
    else GLOG("[WARNING] Atom particle object not found for ferdiad");

    GameObject* smokeObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(smokeParticleName);
    if (smokeObject)
    {
        smokeParticle = smokeObject->GetComponent<ParticleSystemComponent*>();
        if (smokeParticle) smokeParticle->StopInstances();
        else GLOG("[WARNING] Particle component smoke not found for ferdiad");
    }
    else GLOG("[WARNING] Smoke particle object not found for ferdiad");

    GameObject* chargeShieldObject =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(chargeShieldParticleName);
    if (chargeShieldObject)
    {
        chargeShieldParticle = chargeShieldObject->GetComponent<ParticleSystemComponent*>();
        if (chargeShieldParticle) chargeShieldParticle->StopInstances();
        else GLOG("[WARNING] Particle component charge shield not found for ferdiad");
    }
    else GLOG("[WARNING] Charge shield particle object not found for ferdiad");

    GameObject* energyBlastObject1 =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(energyBlastParticleName + std::to_string(1));
    if (energyBlastObject1)
    {
        energyBlastParticle1 = energyBlastObject1->GetComponent<ParticleSystemComponent*>();
        if (energyBlastParticle1) energyBlastParticle1->StopInstances();
        else GLOG("[WARNING] Particle component energy blast 1 not found for ferdiad");
    }
    else GLOG("[WARNING] Energy blast 1 particle object not found for ferdiad");

    GameObject* energyBlastObject2 =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(energyBlastParticleName + std::to_string(2));
    if (energyBlastObject2)
    {
        energyBlastParticle2 = energyBlastObject2->GetComponent<ParticleSystemComponent*>();
        if (energyBlastParticle2) energyBlastParticle2->StopInstances();
        else GLOG("[WARNING] Particle component energy blast 2 not found for ferdiad");
    }
    else GLOG("[WARNING] Energy blast 2 particle object not found for ferdiad");

    GameObject* energyBlastObject3 =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(energyBlastParticleName + std::to_string(3));
    if (energyBlastObject3)
    {
        energyBlastParticle3 = energyBlastObject3->GetComponent<ParticleSystemComponent*>();
        if (energyBlastParticle3) energyBlastParticle3->StopInstances();
        else GLOG("[WARNING] Particle component energy blast 3 not found for ferdiad");
    }
    else GLOG("[WARNING] Energy blast 3 particle object not found for ferdiad");

    GameObject* energyBlastObject4 =
        AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(energyBlastParticleName + std::to_string(4));
    if (energyBlastObject4)
    {
        energyBlastParticle4 = energyBlastObject4->GetComponent<ParticleSystemComponent*>();
        if (energyBlastParticle4) energyBlastParticle4->StopInstances();
        else GLOG("[WARNING] Particle component energy blast 4 not found for ferdiad");
    }
    else GLOG("[WARNING] Energy blast 4 particle object not found for ferdiad");

    GameObject* arenaGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("arena");
    if (arenaGO)
    {
        ScriptComponent* sc = arenaGO->GetComponent<ScriptComponent*>();
        if (sc && sc->GetScriptByType<BossMirage>())
        {
            bossMirageScript = sc->GetScriptByType<BossMirage>();
        }
        else GLOG("[WARNING] Not mirage script component found for ferdiad")
    }
    else GLOG("[WARNING] Boss arena not found for mirage");

    return true;
}

void Boss::Update(float deltaTime)
{
    if (stopLogic && changeScene && !changeScene->IsEnabled())
    {
        timerToChangeScene += deltaTime;
        if (timerToChangeScene >= delayToChangeScene) changeScene->SetEnabled(true);
    }

    if (!agentAI || stopLogic) return;

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

    if (highlightActivated) highlightTimer += deltaTime;
    if (highlightActivated && doTaunt && highlightTimer >= highlightDelay)
    {
        ChooseNextState();
    }
    else if (highlightActivated && highlightTimer >= highlightDelay * 2)
    {
        highlightActivated = false;
        playedHighlight    = true;
    }
}

void Boss::OnPlayerExitLocation()
{
    waiting = true;
}

void Boss::OnPlayerEnterLocation()
{
    waiting = false;

    doTaunt = true;
    // agentAI->ResetAngularSpeed(); // in case doTaunt not used

    if (firstTimeEntering && healthBarBase)
    {
        healthBarBase->SetEnabled(true);
        if (armorBarFill) armorBarFill->SetFillAmount(1.0f);
        if (healthBarFill) healthBarFill->SetFillAmount(1.0f);

        firstTimeEntering = false;
    }
}

void Boss::PlayHighlightSequence()
{
    doTaunt            = true;
    highlightActivated = true;
}

void Boss::DisableBlastArea()
{
    if (blastArea) blastArea->SetEnabled(false);
    blastHit      = true;
    blastHitTimer = 0.0f;
}

void Boss::OnDeath()
{
    stateEnter   = true;
    currentState = BossStates::Death;
}

void Boss::OnDamageTaken(int amount)
{
    // TODO: particles? and animation

    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_HURT);

    if (currentAction == BossActions::Idle || currentAction == BossActions::Chase ||
        currentAction == BossActions::Waiting)
    {
        agentAI->PauseMovement();

        float3 forward = -parent->GetGlobalTransform().WorldZ().Normalized();

        float dot      = hitCollisionNormal.Dot(forward);

        int num        = uniformGetHit(rng);

        if (dot == 0.0f)
        {
            dot = (num == 1) ? 1.0f : -1.0f;
        }

        if (dot > 0.0f)
        {
            switch (num)
            {
            case 1:
                if (animComponent) animComponent->UseTrigger("GetHit1");
                currentAction = BossActions::GetHit1;
                break;
            case 2:
                if (animComponent) animComponent->UseTrigger("GetHit2");
                currentAction = BossActions::GetHit2;
                break;
            default:
                GLOG("ERROR: Ferdiad forward hit anim");
                break;
            }
        }
        else
        {
            switch (num)
            {
            case 1:
                if (animComponent) animComponent->UseTrigger("GetHit1Behind");
                currentAction = BossActions::GetHit1Behind;
                break;
            case 2:
                if (animComponent) animComponent->UseTrigger("GetHit2Behind");
                currentAction = BossActions::GetHit2Behind;
                break;
            default:
                GLOG("ERROR: Ferdiad forward hit anim");
                break;
            }
        }
    }

    if (!armorBarFill || !healthBarFill) return;

    if (currentHealth > phase2)
    {
        armorBarFill->SetFillAmount(
            static_cast<float>(currentHealth - phase2) / static_cast<float>(maxHealth - phase2)
        );
    }
    else
    {
        armorBarFill->SetFillAmount(0.0f);

        healthBarFill->SetFillAmount(static_cast<float>(currentHealth) / static_cast<float>(phase2));
    }
}

void Boss::HandleState(float deltaTime)
{
    if (!mirageActivated && currentHealth <= mirageActivation[phase - 1])
    {
        lastState    = currentState;
        stateEnter   = true;
        currentState = BossStates::Mirage;
    }

    if (phase != 3 && currentHealth <= phaseSwap[phase - 1])
    {
        lastState    = currentState;
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

    case BossStates::Death:
        Death(deltaTime);
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
    if (currentAction == BossActions::Chase || currentAction == BossActions::Return) runTimer += deltaTime;
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
        shieldStrikesRate = 95;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Near:
        shieldStrikesRate  = 80;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate  = 50;
        overheadStrikeRate = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate  = 40;
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
        shieldStrikesRate = 90;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Near:
        shieldStrikesRate = 60;
        shieldBlastRate   = 70;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate = 50;
        shieldBlastRate   = 70;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate = 30;
        shieldBlastRate   = 75;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Far:
        shieldStrikesRate = 20;
        shieldBlastRate   = 85;
        waterSpoutsRate   = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate = 10;
        shieldBlastRate   = 75;
        waterSpoutsRate   = 100;
        break;
    }
    // FOR TESTING
    // waterSpoutsRate   = -1;
    //shieldStrikesRate = -1;
    //shieldBlastRate   = -1;

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
        shieldStrikesRate  = 80;
        overheadStrikeRate = 85;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Near:
        shieldStrikesRate  = 50;
        overheadStrikeRate = 70;
        shieldBlastRate    = 80;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Medium:
        shieldStrikesRate  = 30;
        overheadStrikeRate = 60;
        shieldBlastRate    = 80;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Distant:
        shieldStrikesRate  = 15;
        overheadStrikeRate = 45;
        shieldBlastRate    = 85;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Far:
        shieldBlastRate    = 10;
        overheadStrikeRate = 40;
        shieldBlastRate    = 85;
        waterSpoutsRate    = 100;
        break;

    case BossDistance::Farther:
        shieldStrikesRate  = 5;
        overheadStrikeRate = 25;
        shieldBlastRate    = 85;
        waterSpoutsRate    = 100;
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

void Boss::Death(float deltaTime)
{
    if (stateEnter)
    {
        stateEnter        = false;
        actionTriggerDone = false;
        currentAction     = BossActions::Death;
    }

    switch (currentAction)
    {
    case BossActions::Death:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;

            agentAI->PauseMovement();

            if (playerScript) playerScript->RemoveEnemy();

            ResetValues(false);

            DeleteColliders();

            if (animComponent) animComponent->UseTrigger("Death");
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_DEATH);
        }

        if (animComponent && animComponent->IsFinished())
        {
            stopLogic = true;

            if (healthBarBase) healthBarBase->SetEnabled(false);
        }
        break;
    default:
        break;
    }
}

void Boss::DeleteColliders()
{
    if (closeArea)
    {
        SphereColliderComponent* closeAreaCollider = closeArea->GetComponent<SphereColliderComponent*>();
        if (closeAreaCollider)
        {
            closeAreaCollider->DeleteRigidBody();
            closeAreaCollider->SetEnabled(false);
        }
    }

    if (bigArea)
    {
        SphereColliderComponent* bigAreaCollider = bigArea->GetComponent<SphereColliderComponent*>();
        if (bigAreaCollider)
        {
            bigAreaCollider->DeleteRigidBody();
            bigAreaCollider->SetEnabled(false);
        }
    }

    if (blastArea)
    {
        CubeColliderComponent* blastAreaCollider = blastArea->GetComponent<CubeColliderComponent*>();
        if (blastAreaCollider)
        {
            blastAreaCollider->DeleteRigidBody();
            blastAreaCollider->SetEnabled(false);
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
        ChooseNextState();
    }
    else if (playedHighlight)
    {
        agentAI->ResumeMovement();
        if (waiting) agentAI->SetAngularSpeed(0.5f);
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
    if (playedHighlight) agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

    if (animComponent && animComponent->IsFinished())
    {
        stateEnter   = true;
        currentState = BossStates::Idle;
    }
}

void Boss::Run()
{
    if (!actionTriggerDone)
    {
        runTimer = 0.0f;
        animComponent->UseTrigger("Run");
    }

    if (runTimer > stepTime && audio)
    {
        int num = uniformSteps(rng);
        AkUniqueID eventID;

        switch (num)
        {
        case 1:
            eventID = AK::EVENTS::PLAY_SFX_FERDIAD_STEPS_01;
            break;
        case 2:
            eventID = AK::EVENTS::PLAY_SFX_FERDIAD_STEPS_02;
            break;
        case 3:
            eventID = AK::EVENTS::PLAY_SFX_FERDIAD_STEPS_03;
            break;

        default:
            GLOG("ERROR: Ferdiad steps audio")
            break;
        }

        audio->EmitEvent(eventID);

        runTimer = 0.0f;
    }
}

void Boss::ShieldStrikes(float deltaTime)
{
    if (!weaponCollider) ChooseNextState();

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
            Run();
            actionTriggerDone = true;
            chaseTimer        = 0.0f;
        }
        else
        {
            Run();
            chaseTimer += deltaTime;
        }

        agentAI->SetPathNavigation(character->GetLastPosition());

        if (CheckDistanceWithPlayer() == PlayerDistances::Close)
        {
            if (shieldStrikeLastAction == 1) currentAction = BossActions::Combo2;
            else if (shieldStrikeLastAction == 2) currentAction = BossActions::Combo3;
            else currentAction = BossActions::Combo1;
            actionTriggerDone = false;
        }
        else if (chaseTimer >= chaseTimeLimit)
        {
            agentAI->PauseMovement();
            stateEnter   = true;
            currentState = ChooseAlternativeState();
            return;
        }

        if (shieldStrikeLastAction != 0)
        {
            switch (CheckDistance()) // if far change mechanic
            {
            case BossDistance::Distant:
            case BossDistance::Far:
            case BossDistance::Farther:
                agentAI->PauseMovement();
                stateEnter   = true;
                currentState = ChooseAlternativeState();
                break;
            default:
                break;
            }
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
            audioPlayed            = false;

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        if (attackTimer >= attackHitboxDelay - 0.1f && !audioPlayed)
        {
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_NORMALATTACK_01);

            audioPlayed = true;
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
            audioPlayed            = false;

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        if (attackTimer >= attackHitboxDelay - 0.1f && !audioPlayed)
        {
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_NORMALATTACK_02);

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);

            audioPlayed = true;
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
            audioPlayed            = false;

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);
        }
        else if (!weaponCollider->GetEnabled())
        {
            agentAI->ResumeMovement();
        }

        if (attackTimer >= attackHitboxDelay - 0.1f && !audioPlayed)
        {
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_NORMALATTACK_03);

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);

            audioPlayed = true;
        }

        if (animComponent && animComponent->IsFinished())
        {
            audioPlayed       = false;
            actionTriggerDone = false;
            StopAttacking();
            ChooseNextState();
        }
        break;

    case BossActions::GetHit1:
    case BossActions::GetHit2:
    case BossActions::GetHit1Behind:
    case BossActions::GetHit2Behind:
        if (animComponent && animComponent->IsFinished())
        {
            agentAI->ResumeMovement();
            currentAction = BossActions::Chase;
            runTimer      = 0.0f;
            if (animComponent) animComponent->UseTrigger("Run");
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
        if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(false);
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

        if (runesUV) runesUV->Reset();
        if (runesLightsUV) runesLightsUV->Reset();
        if (dashLightsShieldUV) dashLightsShieldUV->Reset();
        if (dashShieldExpansionUV) dashShieldExpansionUV->Reset();
        if (attackLightingsUV) attackLightingsUV->Reset();
        if (attackEnergyUV) attackEnergyUV->Reset();
        if (attackExplosionUV) attackExplosionUV->Reset();
        if (smallExpansionUV) smallExpansionUV->Reset();
        if (bigExpansionUV) bigExpansionUV->Reset();
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

            if (runesUV) runesUV->SetPaused(false);
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

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (animComponent && animComponent->IsFinished())
        {
            currentAction     = BossActions::Dash;
            actionTriggerDone = false;

            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_AOEDASH);
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
            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(false);
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

            if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(true);

            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_AOEATTACK);
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

    case BossActions::GetHit1:
    case BossActions::GetHit2:
    case BossActions::GetHit1Behind:
    case BossActions::GetHit2Behind:
        if (animComponent && animComponent->IsFinished()) currentAction = BossActions::Waiting;

        break;

    default:
        GLOG("Error: OverheadStrike")
        break;
    }
}

void Boss::StartDash()
{
    isDashing         = true;

    float3 bossPos    = parent->GetGlobalTransform().TranslatePart();
    float3 playerPos  = character->GetLastPosition();

    bossPos.y         = 0.0f;
    playerPos.y       = 0.0f;

    dashDistance      = (playerPos - bossPos).Length();
    dashDirection     = (playerPos - bossPos).Normalized();

    // GLOG("Distance: %.2f", dashDistance);
    // GLOG("Direction: %.2f %.2f %.2f", dashDirection.x, dashDirection.y, dashDirection.z);

    dashSpeed         = dashDistance / dashDuration;
    dashTimeRemaining = dashDuration;

    dashStartPosLocal = parent->GetLocalTransform().TranslatePart();

    // GLOG("Speed: %.2f", dashSpeed);
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
        // if (chargeShieldParticle) chargeShieldParticle->Init();
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
            // if (chargeShieldParticle) chargeShieldParticle->StopInstances();
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

        if (attackEnergyUV) attackEnergyUV->SetPaused(false);
        if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(false);

        ChooseNextState();
    }
}

BossDistance Boss::CheckDistance() const
{
    if (character == nullptr) return BossDistance::None;

    float distance = character->GetLastPosition().Distance(parent->GetGlobalTransform().TranslatePart());
    if (distance <= rangeAIAttack) return BossDistance::Close;
    else if (distance <= 6.0f) return BossDistance::Near;
    else if (distance <= 9.0f) return BossDistance::Medium;
    else if (distance <= 12.0f) return BossDistance::Distant;
    else if (distance <= 15.0f) return BossDistance::Far;
    else return BossDistance::Farther;
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
        ResetValues(false);
        mirageActivated = true;
        stateEnter      = false;
        agentAI->PauseMovement();
        currentAction = BossActions::Start;

        if (invulnerableNoisefallUV) invulnerableNoisefallUV->Reset();
        if (invulnerableWavesUV) invulnerableWavesUV->Reset();

        if (invulnerablePlaneWaterMesh) invulnerablePlaneWaterMesh->SetEnabled(true);
        if (invulnerablePlaneWaterAnimation) invulnerablePlaneWaterAnimation->OnPlay(true);

        if (invulnerableBarrierScript) invulnerableBarrierScript->SetEnabled(true);

        if (invulnerableShieldMesh) invulnerableShieldMesh->SetEnabled(true);
        if (invulnerableShieldAnimation) invulnerableShieldAnimation->OnPlay(true);

        if (invulnerableNoisefallScript) invulnerableNoisefallScript->SetEnabled(true);
        if (invulnerableWavesScript) invulnerableWavesScript->SetEnabled(true);
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

            bossMirageScript->StartSequence(phase);
        }

        if ((int)bossMirageScript->GetSequenceState() == 0)
        {
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
            if (invulnerablePlaneWaterMesh) invulnerablePlaneWaterMesh->SetEnabled(false);
            if (invulnerablePlaneWaterAnimation) invulnerablePlaneWaterAnimation->OnStop();

            if (invulnerableBarrierScript) invulnerableBarrierScript->SetEnabled(false);

            if (invulnerableShieldMesh) invulnerableShieldMesh->SetEnabled(false);
            if (invulnerableShieldAnimation) invulnerableShieldAnimation->OnStop();

            if (invulnerableNoisefallScript) invulnerableNoisefallScript->SetEnabled(false);
            if (invulnerableWavesScript) invulnerableWavesScript->SetEnabled(false);

            agentAI->ResumeMovement();
            actionTriggerDone = false;

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
    if (waterSpouts.empty()) ChooseNextState();

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
        GLOG("Error: Ferdiad WaterSpouts");
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
    if (blastArea) blastArea->SetEnabled(false);

    if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(false);

    if (invulnerablePlaneWaterMesh) invulnerablePlaneWaterMesh->SetEnabled(false);
    if (invulnerablePlaneWaterAnimation) invulnerablePlaneWaterAnimation->OnStop();
    if (invulnerableBarrierScript) invulnerableBarrierScript->SetEnabled(false);
    if (invulnerableShieldMesh) invulnerableShieldMesh->SetEnabled(false);
    if (invulnerableShieldAnimation) invulnerableShieldAnimation->OnStop();
    if (invulnerableNoisefallScript) invulnerableNoisefallScript->SetEnabled(false);
    if (invulnerableWavesScript) invulnerableWavesScript->SetEnabled(false);

    if (emessiveVFXMesh) emessiveVFXMesh->SetEnabled(false);

    if (runesScript) runesScript->SetEnabled(false);
    if (runesUV) runesUV->SetPaused(false);

    if (runesLightsScript) runesLightsScript->SetEnabled(false);
    if (dashGroundMesh) dashGroundMesh->SetEnabled(false);
    if (dashEnergyMesh) dashEnergyMesh->SetEnabled(false);
    if (dashLightsShieldScript) dashLightsShieldScript->SetEnabled(false);
    if (dashShieldExpansionScript) dashShieldExpansionScript->SetEnabled(false);

    if (attackLightingsScript) attackLightingsScript->SetEnabled(false);
    if (attackEnergyScript) attackEnergyScript->SetEnabled(false);
    if (attackEnergyUV) attackEnergyUV->SetPaused(false);

    if (attackExplosionScript) attackExplosionScript->SetEnabled(false);
    if (bigExpansionScript) bigExpansionScript->SetEnabled(false);
    if (smallExpansionScript) smallExpansionScript->SetEnabled(false);
    if (atomParticle) atomParticle->StopInstances();
    if (smokeParticle) smokeParticle->StopInstances();
    if (chargeShieldParticle) chargeShieldParticle->StopInstances();

    if (blastPreSpriteScript) blastPreSpriteScript->SetEnabled(false);
    if (blastEnergySpriteScript) blastEnergySpriteScript->SetEnabled(false);
    if (blastSpriteScript) blastSpriteScript->SetEnabled(false);
    if (blastSpriteScript2) blastSpriteScript2->SetEnabled(false);
    if (energyBlastParticle1) energyBlastParticle1->StopInstances();
    if (energyBlastParticle2) energyBlastParticle2->StopInstances();
    if (energyBlastParticle3) energyBlastParticle3->StopInstances();
    if (energyBlastParticle4) energyBlastParticle4->StopInstances();

    agentAI->ResetAngularSpeed();
    agentAI->SetFreeMove(false);
}

void Boss::ShieldBlast(float deltaTime)
{
    if (!blastArea) ChooseNextState();

    if (stateEnter)
    {
        stateEnter        = false;
        actionTriggerDone = false;
        currentAction     = BossActions::Load;

        blastHitTimer     = 0.0f;
        blastHit          = false;

        if (blastPreSpritesheet) blastPreSpritesheet->Reset();
        if (blastEnergySpritesheet) blastEnergySpritesheet->Reset();
        if (blastSpritesheet) blastSpritesheet->Reset();
        if (blastSpritesheet2) blastSpritesheet2->Reset();
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

            audioPlayed = false;
        }

        if (attackTimer >= blastHitboxDelay - 0.2f)
        {
            agentAI->SetAngularSpeed(1.0f);
        }
        else if (attackTimer >= blastHitboxDelay - 0.4f)
        {
            agentAI->SetAngularSpeed(2.0f);
        }
        else if (attackTimer >= 0.7f)
        {
            agentAI->SetAngularSpeed(3.0f);
        }
        else if (attackTimer >= 0.5f)
        {
            agentAI->SetAngularSpeed(4.0f);
            animComponent->OnPause();
            if (!audioPlayed && audio)
            {
                audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_RANGEATTACKSTART);

                audioPlayed = true;
            }
        }
        else if (attackTimer >= 0.3f)
        {
            agentAI->SetAngularSpeed(5.0f);

            blastPreSpriteScript->SetEnabled(true);
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (blastPreSpritesheet && blastPreSpritesheet->AlmostFinished(6, 3))
        {
            if (blastEnergySpriteScript) blastEnergySpriteScript->SetEnabled(true);
        }

        if (blastPreSpritesheet && blastPreSpritesheet->Finished())
        {
            if (blastPreSpriteScript) blastPreSpriteScript->SetEnabled(false);

            actionTriggerDone = false;
            currentAction     = BossActions::Shoot;
        }
        break;

    case BossActions::Shoot:
        if (!actionTriggerDone)
        {
            actionTriggerDone = true;

            if (blastArea) blastArea->SetEnabled(true);

            if (blastSpriteScript) blastSpriteScript->SetEnabled(true);
            if (blastSpriteScript2) blastSpriteScript2->SetEnabled(true);
            if (energyBlastParticle1) energyBlastParticle1->Init();
            if (energyBlastParticle2) energyBlastParticle2->Init();
            if (energyBlastParticle3) energyBlastParticle3->Init();
            if (energyBlastParticle4) energyBlastParticle4->Init();

            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_FERDIAD_RANGEATTACK);
        }

        if (blastSpritesheet && blastSpritesheet->AlmostFinished(6, 5))
        {
            blastArea->SetEnabled(false);
            if (energyBlastParticle1) energyBlastParticle1->StopInstances();
            if (energyBlastParticle2) energyBlastParticle2->StopInstances();
            if (energyBlastParticle3) energyBlastParticle3->StopInstances();
            if (energyBlastParticle4) energyBlastParticle4->StopInstances();
        }
        else if (blastHit)
        {
            blastHitTimer += deltaTime;
            if (blastHitTimer >= blastAreaDisabledLimit)
            {
                if (blastArea) blastArea->SetEnabled(true);
                blastHit = false;
            }
        }

        agentAI->LookAtMovement(character->GetLastPosition(), deltaTime);

        if (blastSpritesheet && blastSpritesheet->Finished())
        {
            animComponent->OnResume();

            if (blastArea) blastArea->SetEnabled(false);

            if (blastEnergySpriteScript) blastEnergySpriteScript->SetEnabled(false);
            if (blastSpriteScript) blastSpriteScript->SetEnabled(false);
            if (blastSpriteScript2) blastSpriteScript2->SetEnabled(false);

            agentAI->ResetAngularSpeed();
            StopAttacking();

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
    BossStates actualState = currentState;
    if (actualState == BossStates::Mirage || actualState == BossStates::ChangePhase) actualState = lastState;

    if (currentState == BossStates::WaterSpouts && newState == currentState)
    {
        repeatedState = maxRepeats;
    }

    if (newState == actualState)
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
            Run();
            actionTriggerDone = true;
            agentAI->ResumeMovement();
        }
        else Run();

        agentAI->SetPathNavigation(startPos);

        if (CheckDistanceWithPoint(startPos))
        {
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

        if (invulnerableNoisefallUV) invulnerableNoisefallUV->Reset();
        if (invulnerableWavesUV) invulnerableWavesUV->Reset();

        if (invulnerablePlaneWaterMesh) invulnerablePlaneWaterMesh->SetEnabled(true);
        if (invulnerablePlaneWaterAnimation) invulnerablePlaneWaterAnimation->OnPlay(true);

        if (invulnerableBarrierScript) invulnerableBarrierScript->SetEnabled(true);

        if (invulnerableShieldMesh) invulnerableShieldMesh->SetEnabled(true);
        if (invulnerableShieldAnimation) invulnerableShieldAnimation->OnPlay(true);

        if (invulnerableNoisefallScript) invulnerableNoisefallScript->SetEnabled(true);
        if (invulnerableWavesScript) invulnerableWavesScript->SetEnabled(true);
    }

    if (animComponent && animComponent->IsFinished())
    {
        if (invulnerablePlaneWaterMesh) invulnerablePlaneWaterMesh->SetEnabled(false);
        if (invulnerablePlaneWaterAnimation) invulnerablePlaneWaterAnimation->OnStop();
        if (invulnerableBarrierScript) invulnerableBarrierScript->SetEnabled(false);
        if (invulnerableShieldMesh) invulnerableShieldMesh->SetEnabled(false);
        if (invulnerableShieldAnimation) invulnerableShieldAnimation->OnStop();
        if (invulnerableNoisefallScript) invulnerableNoisefallScript->SetEnabled(false);
        if (invulnerableWavesScript) invulnerableWavesScript->SetEnabled(false);

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
        GLOG("ERROR: Ferdiad available states")
        return phase3States;
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

    case BossActions::GetHit1:
        return "GetHit1";

    case BossActions::GetHit2:
        return "GetHit2";

    case BossActions::GetHit1Behind:
        return "GetHit1Behind";

    case BossActions::GetHit2Behind:
        return "GetHit2Behind";

    default:
        return "ERROR: NO ACTION";
    }
}