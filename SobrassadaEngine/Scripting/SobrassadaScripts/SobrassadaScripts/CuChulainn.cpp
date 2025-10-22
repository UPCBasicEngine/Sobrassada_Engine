#include "pch.h"

#include "AbilityIconFill.h"
#include "Application.h"
#include "AttackVfxSpritesheet.h"
#include "BarFill.h"
#include "CameraComponent.h"
#include "CameraMovement.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DamageMask.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameSession.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "MovingUVTransparent.h"
#include "MusicManager.h"
#include "ParticleSystemComponent.h"
#include "ProjectModule.h"
#include "Projectile.h"
#include "RaycastController.h"
#include "ResourceAnimation.h"
#include "ResourceMaterial.h"
#include "ResourceStateMachine.h"
#include "ResourcesModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Standalone/AnimController.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"
#include "Standalone/UI/Transform2DComponent.h"
#include "UIFadeInOut.h"
#include "UISpritesheet.h"

#include "Math/Quat.h"
#include "SDL.h"
#include "Wwise_IDs.h"

extern "C" void GO_RequestGameOver();
extern bool gGameOverActive;

CharacterControllerComponent* character = nullptr;
CuChulainn* playerScript                = nullptr;

CuChulainn::CuChulainn(GameObject* parent)
    : Character(parent, 5, 1, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, CharacterType::CuChulainn)
{
    currentHealth = 3; // mainChar starts low hp

    fields.push_back({InspectorField::FieldType::Spacing, nullptr});
    fields.push_back({InspectorField::FieldType::Text, (void*)"CuChulainn parameters"});
    fields.push_back({"God Mode", InspectorField::FieldType::Bool, &godMode});
    fields.push_back({"Default speed", InspectorField::FieldType::Float, &defaultSpeed, 0.0f, 10.0f});
    fields.push_back({"Step time", InspectorField::FieldType::Float, &stepTime, 0.0f, 1.0f});
    fields.push_back({"Camera Object Name", InspectorField::FieldType::InputText, &cameraName});
    fields.push_back({"Spear Projectile Name", InspectorField::FieldType::InputText, &spearName});
    fields.push_back({"Range attack cooldown", InspectorField::FieldType::Float, &throwCooldown, 0.0f, 2.0f});
    fields.push_back({"Dash cooldown", InspectorField::FieldType::Float, &dashCooldown, 0.0f, 5.0f});
    fields.push_back({"Dash Icon Name", InspectorField::FieldType::InputText, &dashIconName});
    fields.push_back({"Health Bar Name", InspectorField::FieldType::InputText, &healthBarName});
    fields.push_back({"Melee VFX delay", InspectorField::FieldType::Float, &meleeVfxDelay, 0.0f, 1.0f});
    fields.push_back({"Time Stop on hit duration", InspectorField::FieldType::Float, &hitTimeStopDuration, 0.0f, 1.0f});
    fields.push_back(
        {"Time Stop on death duration", InspectorField::FieldType::Float, &deathTimeStopDuration, 0.0f, 1.0f}
    );

    // Unlocked abilities
    fields.push_back({InspectorField::FieldType::Text, (void*)"Unlocked Abilities from Start"});
    fields.push_back({"Dash unlocked", InspectorField::FieldType::Bool, &dashUnlocked});
    fields.push_back({"Ultimate unlocked", InspectorField::FieldType::Bool, &ultimateUnlocked});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Ultimate parameters"});
    fields.push_back({"Ultimate object", InspectorField::FieldType::InputText, &ultimateName});
    fields.push_back({"Ultimate damage", InspectorField::FieldType::Int, &ultimateDamage, 0.0f, 5.0f});
    fields.push_back({"Ultimate cooldown", InspectorField::FieldType::Float, &ultimateCd, 0.0f, 5.0f});
    fields.push_back({"Ultimate Animation delay", InspectorField::FieldType::Float, &ultimateAnimationDelay, 0.0f, 5.0f}
    );
    fields.push_back({"Ultimate Animation speed", InspectorField::FieldType::Float, &ultimateSpeed, 0.1f, 5.0f});
    fields.push_back({"Ultimate hitbox delay", InspectorField::FieldType::Float, &ultimateHitboxDelay, 0.0f, 5.0f});
    fields.push_back({"Ultimate hitbox duration", InspectorField::FieldType::Float, &ultimateHitboxDuration, 0.0f, 5.0f}
    );
    fields.push_back({"Ultimate Icon Name", InspectorField::FieldType::InputText, &ultimateIconName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Charged attack parameters"});
    fields.push_back({"Charged Attack object", InspectorField::FieldType::InputText, &chargedAttackName});
    fields.push_back({"Attack charging duration", InspectorField::FieldType::Float, &chargeDuration, 0.0f, 10.0f});
    fields.push_back({"Charge threshold", InspectorField::FieldType::Float, &chargeThreshold, 0.0f, 10.0f});
    fields.push_back({"Charged Attack damage", InspectorField::FieldType::Int, &chargedAttackDamage, 0.0f, 5.0f});
    fields.push_back(
        {"Charged Attack hitbox delay", InspectorField::FieldType::Float, &chargedAttackHitboxDelay, 0.0f, 5.0f}
    );
    fields.push_back(
        {"Charged Attack hitbox duration", InspectorField::FieldType::Float, &chargedAttackHitboxDuration, 0.0f, 5.0f}
    );

    fields.push_back({InspectorField::FieldType::Text, (void*)"Curse parameters"});
    fields.push_back({"Curse duration", InspectorField::FieldType::Float, &curseDuration, 0.0f, 100.0f});
    fields.push_back({"Curse speed", InspectorField::FieldType::Float, &curseSpeed, 0.0f, 100.0f});
    fields.push_back({"Player material", InspectorField::FieldType::Resource, &playerMaterial});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Healing"});
    fields.push_back({"Take mushroom cooldown", InspectorField::FieldType::Float, &takeMushroomCd, 0.0f, 5.0f});
    fields.push_back({"Mushroom healing", InspectorField::FieldType::Int, &mushroomHeal, 0.0f, 5.0f});
    fields.push_back({"Heal knockback object", InspectorField::FieldType::InputText, &healKnockbackName});
    fields.push_back({"Heal knockback delay", InspectorField::FieldType::Float, &healKnockbackDelay});

    fields.push_back({InspectorField::FieldType::Text, (void*)"Riastrad parameters"});
    fields.push_back({"Riastrad Bar object", InspectorField::FieldType::InputText, &riastradBarName});
    fields.push_back({"Riastrad Eye object", InspectorField::FieldType::InputText, &riastradEyeName});
    fields.push_back({"Riastrad duration", InspectorField::FieldType::Float, &riastradDuration, 0.0f, 100.0f});
    fields.push_back({"Riastrad movement speed", InspectorField::FieldType::Float, &riastradMovementSpeed, 0.0f, 20.0f}
    );
    fields.push_back(
        {"Riastrad animations speed ratio", InspectorField::FieldType::Float, &riastradAnimationsSpeedRatio, 0.0f, 2.0f}
    );
    fields.push_back({"Riastrad on damage taken", InspectorField::FieldType::Int, &riastradOnDamageTaken, 0, 100});
    fields.push_back({"Riastrad on object destroyed", InspectorField::FieldType::Int, &riastradOnObjectHit, 0, 100});
    fields.push_back({"Riastrad on enemy hit", InspectorField::FieldType::Int, &riastradOnEnemyHit, 0, 100});
    fields.push_back({"Riastrad on enemy defeated", InspectorField::FieldType::Int, &riastradOnEnemyDeath, 0, 100});
    fields.push_back({"Transform VFX Delay", InspectorField::FieldType::Float, &transformVfxDelay, 0.0f, 20.0f});
    fields.push_back({"Riastrad VFX blur", InspectorField::FieldType::InputText, &riastradBlurName});
    fields.push_back({"Riastrad VFX crack", InspectorField::FieldType::InputText, &riastradCrackName});
    fields.push_back({"Riastrad VFX waring", InspectorField::FieldType::InputText, &riastradWarningName});

    fields.push_back({InspectorField::FieldType::Text, (void*)"VFX"});
    fields.push_back({"Aim shadow object", InspectorField::FieldType::InputText, &aimShadowName});
    fields.push_back({"Melee trail object", InspectorField::FieldType::InputText, &meleeTrailName});
    fields.push_back({"Melee VFX Horizontal 1", InspectorField::FieldType::InputText, &attackVfxHorizontal1Name});
    fields.push_back({"Melee VFX Vertical 1", InspectorField::FieldType::InputText, &attackVfxVertical1Name});
    fields.push_back({"Melee VFX Horizontal 2", InspectorField::FieldType::InputText, &attackVfxHorizontal2Name});
    fields.push_back({"Melee VFX Vertical 2", InspectorField::FieldType::InputText, &attackVfxVertical2Name});
    fields.push_back({"Melee VFX Horizontal 3", InspectorField::FieldType::InputText, &attackVfxHorizontal3Name});
    fields.push_back({"Attack VFX Vertical 3", InspectorField::FieldType::InputText, &attackVfxVertical3Name});
    fields.push_back({"Attack VFX Explosion", InspectorField::FieldType::InputText, &attackVfxExplosionName});
    fields.push_back({"ArrowHit VFX object", InspectorField::FieldType::InputText, &arrowHitVfxName});
    fields.push_back({"Arrow Hit VFX duration", InspectorField::FieldType::Float, &arrowHitVfxDuration, 0.1f, 5.0f});

    fields.push_back({"Dash Trail object", InspectorField::FieldType::InputText, &dashTrailName});
    fields.push_back({"Dash decal object", InspectorField::FieldType::InputText, &dashDecalName});
    fields.push_back({"Dash decal disappear", InspectorField::FieldType::Float, &dashDecalTimer, 0.0f, 20.0f});
    fields.push_back({"Heal vfx object", InspectorField::FieldType::InputText, &healVfxName});
    fields.push_back({"Heal particles object", InspectorField::FieldType::InputText, &healParticlesName});
    fields.push_back({"Riastrad VFX object", InspectorField::FieldType::InputText, &riastradVfxName});
    fields.push_back({"Damage Mask", InspectorField::FieldType::InputText, &damageMaskName});
}

bool CuChulainn::Init()
{
    Character::Init();

    playerScript = this;

    character    = parent->GetComponent<CharacterControllerComponent*>();
    if (!character) GLOG("CharacterController component not found for CuChulainn")
    character->SetMaxSpeed(defaultSpeed);

    Scene* scene = AppEngine->GetSceneModule()->GetScene();

    cameraObject = scene->GetGameObjectByName(cameraName);
    if (cameraObject && cameraObject->GetComponent<ScriptComponent*>())
    {
        camera = cameraObject->GetComponent<ScriptComponent*>()->GetScriptByType<CameraMovement>();
        if (!camera) GLOG("[WARNING] No camera found by the name %s", cameraName.c_str());

        // Important: This is in the Init() to avoid normalizing each frame. If the camera changes its angle at some
        // point while you can control the character, this will have to be updated as well
        camFront   = cameraObject->GetGlobalTransform().WorldZ();
        camRight   = cameraObject->GetGlobalTransform().WorldX();

        camFront.y = 0;
        camRight.y = 0;
        camFront.Normalize();
        camRight.Normalize();
    }

    const GameObject* spearObj = scene->GetGameObjectByName(spearName);
    if (spearObj && spearObj->GetComponent<ScriptComponent*>())
    {
        spear = spearObj->GetComponent<ScriptComponent*>()->GetScriptByType<Projectile>();
        if (!spear) GLOG("[WARNING] No projectile found by the name %s", spearName.c_str());
    }

    spearCharacter = scene->GetGameObjectByName(spearNameMesh);
    if (!spearCharacter) GLOG("[WARNING] No spear (non projectile) found for CuChualin")
    else spearCharacter->SetEnabled(true);

    chargedAttackCollider = scene->GetGameObjectByName(chargedAttackName);
    if (!chargedAttackCollider) GLOG("[WARNING] No charge attack found for CuChualin")
    else chargedAttackCollider->SetEnabled(false);

    healKnockback = scene->GetGameObjectByName(healKnockbackName);
    if (!healKnockback) GLOG("[WARNING] No heal knockback found for CuChualin")
    else healKnockback->SetEnabled(false);

    ultimateObject = scene->GetGameObjectByName(ultimateName);
    if (!ultimateObject) GLOG("[WARNING] No ultimate found for CuChulain")
    else ultimateObject->SetEnabled(false);

    aimShadowObject = scene->GetGameObjectByName(aimShadowName);
    if (!aimShadowObject) GLOG("[WARNING] No shadow found for aiming in CuChulain")
    else aimShadowObject->SetEnabled(false);

    meleeTrailObject = scene->GetGameObjectByName(meleeTrailName);
    if (!meleeTrailObject) GLOG("[WARNING] No melee trail found for melee attack in CuChulain")
    else meleeTrailObject->SetEnabled(false);

    arrowHitVfxObject = scene->GetGameObjectByName(arrowHitVfxName);
    if (!arrowHitVfxObject) GLOG("[WARNING] No arrow Hit particles found for Hits in CuChulain")
    else arrowHitVfxObject->SetEnabled(false);

    GameObject* attackVfxObj = scene->GetGameObjectByName(attackVfxHorizontal1Name);
    if (attackVfxObj) attackVfxHorizontal1 = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxHorizontal1) attackVfxHorizontal1->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 1 found for melee attack in CuChulain");

    attackVfxObj = scene->GetGameObjectByName(attackVfxVertical1Name);
    if (attackVfxObj) attackVfxVertical1 = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxVertical1) attackVfxVertical1->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 1 found for melee attack in CuChulain");

    attackVfxObj = scene->GetGameObjectByName(attackVfxHorizontal2Name);
    if (attackVfxObj) attackVfxHorizontal2 = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxHorizontal2) attackVfxHorizontal2->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 2 found for melee attack in CuChulain");

    attackVfxObj = scene->GetGameObjectByName(attackVfxVertical2Name);
    if (attackVfxObj) attackVfxVertical2 = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxVertical2) attackVfxVertical2->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 2 found for melee attack in CuChulain");

    attackVfxObj = scene->GetGameObjectByName(attackVfxHorizontal3Name);
    if (attackVfxObj) attackVfxHorizontal3 = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxHorizontal3) attackVfxHorizontal3->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 3 found for melee attack in CuChulain");

    attackVfxObj = scene->GetGameObjectByName(attackVfxVertical3Name);
    if (attackVfxObj) attackVfxVertical3 = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxVertical3) attackVfxVertical3->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 3 found for melee attack in CuChulain");

    attackVfxObj = scene->GetGameObjectByName(attackVfxExplosionName);
    if (attackVfxObj) attackVfxExplosion = attackVfxObj->GetComponent<ShaderScriptComponent*>();
    if (attackVfxExplosion) attackVfxExplosion->SetEnabled(false);
    else GLOG("[WARNING] No melee VFX 3 found for melee attack in CuChulain");

    arrowHitVfxObject = scene->GetGameObjectByName(arrowHitVfxName);
    if (!arrowHitVfxObject) GLOG("[WARNING] No arrow Hit particles found for Hits in CuChulain")
    else arrowHitVfxObject->SetEnabled(false);

    dashTrail = scene->GetGameObjectByName(dashTrailName);
    if (!dashTrail) GLOG("[WARNING] No dash trail found for CuChulain")
    else dashTrail->SetEnabled(false);

    dashDecal = scene->GetGameObjectByName(dashDecalName);
    if (!dashDecal) GLOG("[WARNING] No dash decal found for CuChulain")
    else dashDecal->SetEnabled(false);

    GameObject* dashVfxObj = scene->GetGameObjectByName(dashSmokeName1);
    if (dashVfxObj)
    {
        dashSmoke1 = dashVfxObj->GetComponent<ShaderScriptComponent*>();
    }
    if (dashSmoke1) dashSmoke1->SetEnabled(false);

    dashVfxObj = scene->GetGameObjectByName(dashSmokeName2);
    if (dashVfxObj)
    {
        dashSmoke2 = dashVfxObj->GetComponent<ShaderScriptComponent*>();
    }
    if (dashSmoke2) dashSmoke2->SetEnabled(false);

    healVfx = scene->GetGameObjectByName(healVfxName);
    if (!healVfx) GLOG("[WARNING] No heal visual found for CuChulain")
    else healVfx->SetEnabled(false);

    healParticles = scene->GetGameObjectByName(healParticlesName);
    if (!healParticles) GLOG("[WARNING] No heal visual found for CuChulain")
    else healParticles->SetEnabled(false);

    // Riastrad VFX
    riastradVfx = scene->GetGameObjectByName(riastradVfxName);
    if (!riastradVfx) GLOG("[WARNING] No riastrad VFX found for CuChulain")

    riastradBlur = scene->GetGameObjectByParentNameAndTargetName(riastradVfxName, riastradBlurName);
    if (!riastradBlur) GLOG("[WARNING] No riastrad Blur VFX found for CuChulain")
    else riastradBlur->SetEnabled(false);

    riastradCrack = scene->GetGameObjectByParentNameAndTargetName(riastradVfxName, riastradCrackName);
    if (!riastradCrack) GLOG("[WARNING] No riastrad Crack VFX found for CuChulain")
    else riastradCrack->SetEnabled(false);

    riastradWarning = scene->GetGameObjectByParentNameAndTargetName(riastradVfxName, riastradWarningName);
    if (!riastradWarning) GLOG("[WARNING] No riastrad Warning VFX found for CuChulain")
    else riastradWarning->SetEnabled(false);

    riastradStars = scene->GetGameObjectByParentNameAndTargetName(riastradVfxName, riastradStarsName);
    if (!riastradStars) GLOG("[WARNING] No riastrad Stars VFX found for CuChulain")
    else riastradStars->SetEnabled(false);

    GameObject* riastradObj = scene->GetGameObjectByName(riastradSmokeName);
    if (riastradObj)
    {
        riastradSmoke = riastradObj->GetComponent<ShaderScriptComponent*>();
    }
    if (riastradSmoke) riastradSmoke->SetEnabled(false);

    riastradObj = scene->GetGameObjectByName(riastradGroundExplosionName);
    if (riastradObj)
    {
        riastradGroundExplosion = riastradObj->GetComponent<ShaderScriptComponent*>();
    }
    if (riastradGroundExplosion) riastradGroundExplosion->SetEnabled(false);

    riastradObj = scene->GetGameObjectByName(riastradBarName);
    if (riastradObj)
    {
        ShaderScriptComponent* shaderScript = riastradObj->GetComponent<ShaderScriptComponent*>();
        if (shaderScript) riastradBar = shaderScript->GetScriptByType<BarFill>();
    }
    if (!riastradBar) GLOG("[WARNING] No riastrad Fill Bar Shader Script found for CuChulain");

    riastradObj = scene->GetGameObjectByName(riastradEyeName);
    if (riastradObj)
    {
        ShaderScriptComponent* shaderScript = riastradObj->GetComponent<ShaderScriptComponent*>();
        if (shaderScript) riastradEye = shaderScript->GetScriptByType<AbilityIconFill>();
    }
    if (!riastradEye) GLOG("[WARNING] No riastrad Eye Shader Script found for CuChulain");

    riastradObj = scene->GetGameObjectByName(riastradVfxBGName);
    if (riastradObj)
    {
        riastradVfxBG = riastradObj->GetComponent<ShaderScriptComponent*>();
    }
    if (riastradVfxBG) riastradVfxBG->SetEnabled(false);
    GLOG("[WARNING] No riastrad Eye BG VFX Shader Script found for CuChulain");

    riastradObj = scene->GetGameObjectByName(riastradVfxFGName);
    if (riastradObj)
    {
        riastradVfxFG = riastradObj->GetComponent<ShaderScriptComponent*>();
    }
    if (riastradVfxFG) riastradVfxFG->SetEnabled(false);
    else GLOG("[WARNING] No riastrad Eye FG VFX Shader Script found for CuChulain");

    riastradObj = scene->GetGameObjectByName(riastradFireUpName);
    if (riastradObj)
    {
        riastradFireUp = riastradObj->GetComponent<ShaderScriptComponent*>();
    }
    if (riastradFireUp) riastradFireUp->SetEnabled(false);

    riastradObj = scene->GetGameObjectByName(riastradFireDownName);
    if (riastradObj)
    {
        riastradFireDown = riastradObj->GetComponent<ShaderScriptComponent*>();
    }
    if (riastradFireDown) riastradFireDown->SetEnabled(false);

    riastradTriggers = scene->GetGameObjectByName(riastradTriggersName);
    if (!riastradTriggers) GLOG("[WARNING] No riastrad triggers HUD element found")
    else riastradTriggers->SetEnabled(false);

    riastradKey = scene->GetGameObjectByName(riastradKeyName);
    if (!riastradKey) GLOG("[WARNING] No riastrad key HUD element found")
    else riastradKey->SetEnabled(false);

    GameObject* healthBarObj = scene->GetGameObjectByName(healthBarName);
    if (healthBarObj)
    {
        ShaderScriptComponent* shaderScript = healthBarObj->GetComponent<ShaderScriptComponent*>();
        if (shaderScript)
        {
            healthBar = shaderScript->GetScriptByType<BarFill>();
            healthBar->SetFillAmount(static_cast<float>(currentHealth) / static_cast<float>(maxHealth));
        }
    }
    if (!healthBar) GLOG("[WARNING] No health Fill Bar Shader Script found for CuChulain");

    hudMushrooms[0] = scene->GetGameObjectByName(hudMushroomName1);
    if (!hudMushrooms[0]) GLOG("[WARNING] No HUD Mushroom 1 found for CuChulain")
    else hudMushrooms[0]->SetEnabled(false);

    hudMushrooms[1] = scene->GetGameObjectByName(hudMushroomName2);
    if (!hudMushrooms[1]) GLOG("[WARNING] No HUD Mushroom 2 found for CuChulain")
    else hudMushrooms[1]->SetEnabled(false);

    hudMushrooms[2] = scene->GetGameObjectByName(hudMushroomName3);
    if (!hudMushrooms[2]) GLOG("[WARNING] No HUD Mushroom 3 found for CuChulain")
    else hudMushrooms[2]->SetEnabled(false);

    GameObject* obj = scene->GetGameObjectByName(hudMushroomUseName1);
    if (obj)
    {
        hudMushroomsUse[0] = obj->GetComponent<ShaderScriptComponent*>();
    }
    if (!hudMushroomsUse[0]) GLOG("[WARNING] No HUD Mushroom Use 1 found for CuChulain")
    else hudMushroomsUse[0]->SetEnabled(false);

    obj = scene->GetGameObjectByName(hudMushroomUseName2);
    if (obj)
    {
        hudMushroomsUse[1] = obj->GetComponent<ShaderScriptComponent*>();
    }
    if (!hudMushroomsUse[1]) GLOG("[WARNING] No HUD Mushroom Use 2 found for CuChulain")
    else hudMushroomsUse[1]->SetEnabled(false);

    obj = scene->GetGameObjectByName(hudMushroomUseName3);
    if (obj)
    {
        hudMushroomsUse[2] = obj->GetComponent<ShaderScriptComponent*>();
    }
    if (!hudMushroomsUse[2]) GLOG("[WARNING] No HUD Mushroom Use 3 found for CuChulain")
    else hudMushroomsUse[2]->SetEnabled(false);

    obj = scene->GetGameObjectByName(hudMushroomPickName1);
    if (obj)
    {
        hudMushroomsPick[0] = obj->GetComponent<ShaderScriptComponent*>();
    }
    if (!hudMushroomsPick[0]) GLOG("[WARNING] No HUD Mushroom Pick 1 found for CuChulain")
    else hudMushroomsPick[0]->SetEnabled(false);

    obj = scene->GetGameObjectByName(hudMushroomPickName2);
    if (obj)
    {
        hudMushroomsPick[1] = obj->GetComponent<ShaderScriptComponent*>();
    }
    if (!hudMushroomsPick[1]) GLOG("[WARNING] No HUD Mushroom Pick 2 found for CuChulain")
    else hudMushroomsPick[1]->SetEnabled(false);

    obj = scene->GetGameObjectByName(hudMushroomPickName3);
    if (obj)
    {
        hudMushroomsPick[2] = obj->GetComponent<ShaderScriptComponent*>();
    }
    if (!hudMushroomsPick[2]) GLOG("[WARNING] No HUD Mushroom Pick 3 found for CuChulain")
    else hudMushroomsPick[2]->SetEnabled(false);

    GameObject* damageMaskObj = scene->GetGameObjectByName(damageMaskName);
    if (damageMaskObj)
    {
        ShaderScriptComponent* shaderScript = damageMaskObj->GetComponent<ShaderScriptComponent*>();
        if (shaderScript)
        {
            damageMask = shaderScript->GetScriptByType<DamageMask>();
            damageMask->SetLife(static_cast<float>(currentHealth));
        }
    }
    if (!damageMask) GLOG("[WARNING] No health Fill Bar Shader Script found for CuChulain");

    damageMaskObj = scene->GetGameObjectByName(damageScratchName1);
    if (damageMaskObj)
    {
        damageScratch[0] = damageMaskObj->GetComponent<ShaderScriptComponent*>();
    }
    if (damageScratch[0]) damageScratch[0]->SetEnabled(false);
    damageMaskObj = scene->GetGameObjectByName(damageScratchName2);
    if (damageMaskObj)
    {
        damageScratch[1] = damageMaskObj->GetComponent<ShaderScriptComponent*>();
    }
    if (damageScratch[1]) damageScratch[1]->SetEnabled(false);
    damageMaskObj = scene->GetGameObjectByName(damageScratchName3);
    if (damageMaskObj)
    {
        damageScratch[2] = damageMaskObj->GetComponent<ShaderScriptComponent*>();
    }
    if (damageScratch[2]) damageScratch[2]->SetEnabled(false);
    damageMaskObj = scene->GetGameObjectByName(damageScratchName4);
    if (damageMaskObj)
    {
        damageScratch[3] = damageMaskObj->GetComponent<ShaderScriptComponent*>();
    }
    if (damageScratch[3]) damageScratch[3]->SetEnabled(false);
    damageMaskObj = scene->GetGameObjectByName(damageScratchName5);
    if (damageMaskObj)
    {
        damageScratch[4] = damageMaskObj->GetComponent<ShaderScriptComponent*>();
    }
    if (damageScratch[4]) damageScratch[4]->SetEnabled(false);

    GameObject* dashIconObj = scene->GetGameObjectByName(dashIconName);

    if (dashIconObj)
    {
        ShaderScriptComponent* shaderScript = dashIconObj->GetComponent<ShaderScriptComponent*>();
        if (shaderScript) dashIcon = shaderScript->GetScriptByType<AbilityIconFill>();
    }
    if (!dashIcon) GLOG("[WARNING] No dash icon Shader Script found for CuChulain");

    GameObject* ultimateIconObj = scene->GetGameObjectByName(ultimateIconName);
    if (ultimateIconObj)
    {
        ShaderScriptComponent* shaderScript = ultimateIconObj->GetComponent<ShaderScriptComponent*>();
        if (shaderScript) ultimateIcon = shaderScript->GetScriptByType<AbilityIconFill>();
    }
    if (!ultimateIcon) GLOG("[WARNING] No dash icon Shader Script found for CuChulain");

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] CuChulainn: No audio component found");

    // Charged attack
    GameObject* chargeSpritesheet = parent->GetChildGameObjectByName(chargeSpritesheetName1);
    if (chargeSpritesheet)
    {
        chargeVfx1 = chargeSpritesheet->GetComponent<ShaderScriptComponent*>();
        if (chargeVfx1) chargeVfx1->SetEnabled(false);
        else GLOG("[WARNING] No charge attack VFX found for CuChulain");
    }

    chargeSpritesheet = parent->GetChildGameObjectByName(chargeSpritesheetName2);
    if (chargeSpritesheet)
    {
        chargeVfx2 = chargeSpritesheet->GetComponent<ShaderScriptComponent*>();
        if (chargeVfx2) chargeVfx2->SetEnabled(false);
        else GLOG("[WARNING] No charge attack VFX found for CuChulain");
    }

    chargeSpritesheet = parent->GetChildGameObjectByName(chargeSpritesheetName3);
    if (chargeSpritesheet)
    {
        chargeVfx3 = chargeSpritesheet->GetComponent<ShaderScriptComponent*>();
        if (chargeVfx3) chargeVfx3->SetEnabled(false);
        else GLOG("[WARNING] No charge attack VFX found for CuChulain");
    }

    chargeSpritesheet = parent->GetChildGameObjectByName(chargeAttackVfxName);
    if (chargeSpritesheet)
    {
        chargedAttackVfx = chargeSpritesheet->GetComponent<ShaderScriptComponent*>();
        if (chargedAttackVfx) chargedAttackVfx->SetEnabled(false);
        else GLOG("[WARNING] No charge attack VFX found for CuChulain");
    }

    // Ultimate
    ultimateGlow = scene->GetGameObjectByParentNameAndTargetName(ultimateName, ultimateGlowName);
    if (!ultimateGlow) GLOG("[WARNING] No ultimate Glow VFX found for CuChulain")
    else ultimateGlow->SetEnabled(false);

    ultimateBlur = scene->GetGameObjectByParentNameAndTargetName(ultimateName, ultimateBlurName);
    if (!ultimateBlur) GLOG("[WARNING] No ultimate Blur VFX found for CuChulain")
    else ultimateBlur->SetEnabled(false);

    ultimateBrust = scene->GetGameObjectByParentNameAndTargetName(ultimateName, ultimateBrustName);
    if (!ultimateBrust) GLOG("[WARNING] No ultimate Brust VFX found for CuChulain")
    else ultimateBrust->SetEnabled(false);

    ultimateCrack = scene->GetGameObjectByParentNameAndTargetName(ultimateName, ultimateCrackName);
    if (!ultimateCrack) GLOG("[WARNING] No ultimate Crack1 VFX found for CuChulain")
    else ultimateCrack->SetEnabled(false);

    ultimateWarning = scene->GetGameObjectByParentNameAndTargetName(ultimateName, ultimateWarningName);
    if (!ultimateWarning) GLOG("[WARNING] No ultimate warning VFX found for CuChulain")
    else ultimateWarning->SetEnabled(false);

    ultimateSpikes = scene->GetGameObjectByParentNameAndTargetName(ultimateName, ultimateSpikesName);
    if (!ultimateSpikes) GLOG("[WARNING] No ultimate spikes VFX found for CuChulain")
    else ultimateSpikes->SetEnabled(false);

    CapsuleColliderComponent* playerCollider = parent->GetComponent<CapsuleColliderComponent*>();
    if (playerCollider)
    {
        GLOG("=== PLAYER COLLIDER INFO ===");
        GLOG("Player collider enabled: %s", playerCollider->GetEnabled() ? "true" : "false");
        GLOG("Player name: %s", parent->GetName().c_str());

        if (parent->HasTag(HashString("Player")))
        {
            GLOG("Player has 'Player' tag: YES");
        }
        else
        {
            GLOG("Player has 'Player' tag: NO - THIS IS A PROBLEM!");
        }
    }
    else
    {
        GLOG("[ERROR] Player has no CapsuleColliderComponent!");
    }
    state                         = CharacterStates::IDLE;

    // Apply saved changes between scenes
    const std::string projectPath = AppEngine->GetProjectModule()->GetLoadedProjectPath();
    const std::string savePath    = SavePlayerData::MakeSavePath(projectPath);

    spawnPos                      = parent->GetGlobalTransform().TranslatePart();

    if (gNewGame)
    {
        gNewGame = false;
        SavePlayerData::DeleteSaveFile(savePath);
    }
    else
    {
        PlayerState loadedPlayerState;
        if (SavePlayerData::LoadPlayerFromFile(loadedPlayerState, savePath)) ApplySavedState(loadedPlayerState);
    }

    for (int i = 0; i < mushrooms; ++i)
    {
        if (hudMushrooms[i]) hudMushrooms[i]->SetEnabled(true);
    }

    return true;
}

void CuChulainn::Update(float deltaTime)
{
    if (state == CharacterStates::DEATH)
    {
        deathTimer                   += deltaTime;

        const bool deathAnimDone      = (animComponent && animComponent->IsFinished());
        constexpr float kGO_MinDelay  = 1.6f;
        constexpr float kGO_MaxDelay  = 3.5f;

        if (pendingGameOver && ((deathAnimDone && deathTimer >= kGO_MinDelay) || (deathTimer >= kGO_MaxDelay)))
        {
            pendingGameOver = false;
            GO_RequestGameOver();
        }

        if (deathTimer > 4.0f && !gGameOverActive) Respawn();
    }

    if (isDead || !character) return;

    if (character->GetInputDown()) GetInputs();
    Character::Update(deltaTime);
    PerformAttack();

    // Heal knockback and VFX
    if (state == CharacterStates::HEAL && healTimer > healKnockbackDelay && !healKnockback->IsEnabled())
    {
        if (healKnockback) healKnockback->SetEnabled(true);
        if (healParticles)
        {
            healParticles->SetEnabled(true);
            healParticles->GetComponent<ParticleSystemComponent*>()->SpawnAllInstances();
        }
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_HEAL);
        Heal(mushroomHeal);
    }

    // RiastradVFX
    if (state == CharacterStates::TRANSFORM)
    {
        if (!riastradCrack->IsEnabled()) EnableRiastradVfx();
    }

    if (ultimateObject && ultimateObject->IsEnabled())
    {
        AnimationComponent* vfxUltimateAnim = ultimateObject->GetComponent<AnimationComponent*>();
        // Ultimate VFX finish animation
        if (vfxUltimateAnim && vfxUltimateAnim->IsFinished())
        {
            vfxUltimateAnim->OnStop();
            if (ultimateSpikes) ultimateSpikes->SetEnabled(false);
            if (ultimateCrack) ultimateCrack->SetEnabled(false);
            ultimateObject->SetEnabled(false);
            ultimateTimer = 0.0f;
            if (meleeTrailObject) meleeTrailObject->SetEnabled(false);

            if (playerAnimHeld && animComponent)
            {
                animComponent->OnResume();
                playerAnimHeld = false;
            }

            controlsLocked = false;
        }
    }

    // Dash decal spawn when in middle of dash
    const float dashDecalTriggerDist = 2.0f;
    if (state == CharacterStates::DASH && dashDecal &&
        parent->GetGlobalTransform().TranslatePart().Distance(lastDashStartPos) > dashDecalTriggerDist)
    {
        // TODO: set dash decal to the final position of player and not to direction
        dashDecal->SetEnabled(true);
        const float3 scale = dashDecal->GetLocalTransform().ExtractScale();
        const Quat rotation =
            Quat::LookAt(float3::unitY, character->GetDashDirection().Normalized(), float3::unitZ, float3::unitY);
        const float3 pos              = lastDashStartPos + 1.5f * character->GetDashDirection().Normalized();
        const float4x4 decalTransform = float4x4::FromTRS(pos, rotation, scale);
        dashDecal->SetLocalTransform(decalTransform);
        dashDecalBufferTimer = dashDecalTimer;
    }

    CheckIsFalling();
    if (dashIcon) dashIcon->SetFillAmount(1.0f - (dashTimer / dashCooldown));
    if (ultimateIcon) ultimateIcon->SetFillAmount(1.0f - (ultimateCdTimer / ultimateCd));

    if (!isDashing && dashTrail && dashTrail->IsEnabled()) dashTrail->SetEnabled(false);

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life           = "Health: " + std::to_string(currentHealth);
        const std::string animState      = "Anim state: " + stateName.GetString();
        const std::string logicState     = "Logic state: " + GetLogicStateName();
        const std::string mushroomsState = "Mushrooms: " + std::to_string(mushrooms);
        const std::string riastradState  = isRiastrad ? "Riastrad active: Yes" : "Riastrad active: No";
        const std::string riastradCharge = "Riastrad meter: " + std::to_string(riastradMeter);

        std::vector<std::pair<std::string, float2>> logs {
            {life,           float2(-80.0f, -140.0f)},
            {animState,      float2(-80.0f, -160.0f)},
            {logicState,     float2(-80.0f, -180.0f)},
            {mushroomsState, float2(-80.0f, -200.0f)},
            {riastradState,  float2(-80.0f, -220.0f)},
            {riastradCharge, float2(-80.0f, -240.0f)},
        };

        RenderDebug(logs, float3(0.0f, 1.0f, 0.0f));
    }
}

void CuChulainn::OnDestroy()
{
    animComponent->GetResourceStateMachine()->ResetClipsSpeed();
}

void CuChulainn::OnDeath()
{
    isAttacking = false;
    deathTimer  = 0.0f;
    character->EnableMovement(false);
    state = CharacterStates::DEATH;

    if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
    if (attackVfxHorizontal1) attackVfxHorizontal1->SetEnabled(false);
    if (attackVfxHorizontal2) attackVfxHorizontal2->SetEnabled(false);
    if (attackVfxHorizontal3) attackVfxHorizontal3->SetEnabled(false);
    if (attackVfxVertical1) attackVfxVertical1->SetEnabled(false);
    if (attackVfxVertical2) attackVfxVertical2->SetEnabled(false);
    if (attackVfxVertical3) attackVfxVertical3->SetEnabled(false);

    if (state == CharacterStates::AIM && camera) camera->EnableAimOffset(false);
    if (animComponent) animComponent->UseTrigger("Death");
    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_DEATH);

    isDead          = true;
    pendingGameOver = true;
}

void CuChulainn::OnDamageTaken(int amount)
{
    // TODO: Update health
    if (camera) camera->StartShake(0.2f, 0.2f);

    if (healthBar) healthBar->SetFillAmount(static_cast<float>(currentHealth) / static_cast<float>(maxHealth));

    if (state == CharacterStates::CHARGING && audio) audio->StopAudio();

    if (audio && currentHealth >= 1) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_HURT);
    AddRiastrad(riastradOnDamageTaken);

    if (arrowVfxIsActive && arrowHitVfxObject && !arrowHitVfxObject->IsEnabled())
    {
        GLOG("Activating arrow VFX - isActive: %s, timer: %f", arrowVfxIsActive ? "true" : "false", arrowHitVfxTimer);

        arrowHitVfxObject->SetEnabled(true);

        ParticleSystemComponent* particleSystem = arrowHitVfxObject->GetComponent<ParticleSystemComponent*>();
        if (particleSystem)
        {
            particleSystem->SpawnAllInstances();
            GLOG("Arrow VFX particles spawned");
        }
    }

    if (damageMask)
    {
        damageMask->SetLife(static_cast<float>(currentHealth));
        damageMask->OnHit();
    }

    int randomNum = rand() % 5;
    if (damageScratch[randomNum])
    {
        damageScratch[randomNum]->SetEnabled(true);
        damageScratch[randomNum]->GetScriptByType<UISpritesheet>()->Reset();
    }

    if (state == CharacterStates::CHARGING || state == CharacterStates::IDLE || state == CharacterStates::RUN ||
        state == CharacterStates::HEAL)
    {
        state = CharacterStates::HURT;
        if (animComponent)
        {
            float x = (float)rand() / RAND_MAX;
            if (x < 0.5f) animComponent->UseTrigger("Hurt");
            else animComponent->UseTrigger("Hurt2");
            character->EnableMovement(false);
        }
    }

    // TODO: Test if hitstop when hit feels nice
    // AppEngine->GetGameTimer()->SetTimeScale(0.0f);
}

void CuChulainn::OnHealed(int amount)
{
    // TODO: play CuChulainn recover sound
    if (healthBar) healthBar->SetFillAmount(static_cast<float>(currentHealth) / static_cast<float>(maxHealth));
    if (damageMask) damageMask->SetLife(static_cast<float>(currentHealth));
}

void CuChulainn::HandleState(float deltaTime)
{
    if (state == CharacterStates::AIM && !desiredAim && !resetWeapon)
    {
        animComponent->OnResume();
        animComponent->UseTrigger("Idle");
        state    = CharacterStates::IDLE;
        aimTimer = 0.0f;
    }

    if (desiredTransform && CanTransform()) ToggleRiastrad();
    else if (desiredDash && CanDash()) Dash();
    else if (desiredHeal && CanHeal()) UseMushroom();
    else if (desiredUltimate && CanUltimate()) UltimateAttack();
    else if (desiredAttack && CanAttack()) Attack(deltaTime);
    else if (desiredAim && CanAim()) Aim(deltaTime);
    else if (attackPressTimer >= chargeThreshold && CanChargeAttack()) ChargeAttack();
    else if (state != CharacterStates::BASIC_ATTACK && !character->IsDashing() && state != CharacterStates::RESPAWN &&
             state != CharacterStates::AIM && state != CharacterStates::FALL && state != CharacterStates::ULTIMATE &&
             state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::CHARGING &&
             state != CharacterStates::HEAL && state != CharacterStates::TRANSFORM && state != CharacterStates::HURT &&
             state != CharacterStates::TAKE_MUSHROOM)
        Move();

    // When finished animation, go back to idle state
    if (animComponent && animComponent->IsFinished())
    {
        if (stateName == HashString("Attack_1") || stateName == HashString("Attack_2") ||
            stateName == HashString("Attack_3") || stateName == HashString("Attack_4"))
        {
            if (isAttacking) comboBufferTimer = 0.1f;
            isAttacking = false;
            if (attackVfxHorizontal1) attackVfxHorizontal1->SetEnabled(false);
            if (attackVfxVertical1) attackVfxVertical1->SetEnabled(false);
            if (attackVfxHorizontal2) attackVfxHorizontal2->SetEnabled(false);
            if (attackVfxVertical2) attackVfxVertical2->SetEnabled(false);
            if (attackVfxHorizontal3) attackVfxHorizontal3->SetEnabled(false);
            if (attackVfxVertical3) attackVfxVertical3->SetEnabled(false);
            // if (attackVfxExplosion) attackVfxExplosion->SetEnabled(false);
        }
        else if (stateName == HashString("Charge"))
        {
            animComponent->UseTrigger("Charge");
        }
        else
        {
            if (state == CharacterStates::HEAL && healVfx) healVfx->SetEnabled(false);
            // if (state == CharacterStates::ULTIMATE &&
            // ultimateObject->GetComponent<AnimationComponent*>()->IsPlaying())
            //{
            //     return;
            // }
            if (state == CharacterStates::CHARGED_ATTACK && meleeTrailObject) meleeTrailObject->SetEnabled(false);
            if (state == CharacterStates::HEAL && healKnockback) healKnockback->SetEnabled(false);
            if (state == CharacterStates::TRANSFORM)
            {
                transformTimer = 0.0f;
                chargedAttackCollider->SetEnabled(false);
                riastradVfx->GetComponent<AnimationComponent*>()->OnStop();
                // riastradVfx->SetEnabled(false);

                if (riastradBlur) riastradBlur->SetEnabled(false);
                if (riastradCrack) riastradCrack->SetEnabled(false);
                if (riastradWarning) riastradWarning->SetEnabled(false);
                if (riastradStars) riastradStars->SetEnabled(false);
            }
            state = CharacterStates::IDLE;
            animComponent->UseTrigger("Idle");
        }
    }
}

void CuChulainn::GetInputs()
{
    if (AppEngine->GetGameTimer()->GetDeltaTime() <= 0.0f) return;
    if (controlsLocked) return;

    const InputModule* input   = AppEngine->GetInputModule();
    const KeyState* keyboard   = input->GetKeyboard();
    const KeyState* mouse      = input->GetMouseButtons();
    const KeyState* controller = input->GetControllerButtons();
    const float2& leftJoystick = input->GetLeftStick();

    float3 direction           = float3::zero;
    if (input->IsUsingKeyboard())
    {

        if (keyboard[SDL_SCANCODE_W] == KEY_REPEAT) direction.z -= 1.0f;
        if (keyboard[SDL_SCANCODE_S] == KEY_REPEAT) direction.z += 1.0f;
        if (keyboard[SDL_SCANCODE_A] == KEY_REPEAT) direction.x -= 1.0f;
        if (keyboard[SDL_SCANCODE_D] == KEY_REPEAT) direction.x += 1.0f;
    }
    else
    {
        direction.x = leftJoystick.x;
        direction.z = leftJoystick.y;

        if (controller[SDL_CONTROLLER_BUTTON_DPAD_LEFT] == KEY_REPEAT) direction.x = -1.0f;
        if (controller[SDL_CONTROLLER_BUTTON_DPAD_UP] == KEY_REPEAT) direction.z = -1.0f;
        if (controller[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] == KEY_REPEAT) direction.x = 1.0f;
        if (controller[SDL_CONTROLLER_BUTTON_DPAD_DOWN] == KEY_REPEAT) direction.z = 1.0f;
    }

    moveFromCollision = (direction.Length() >= 0.55f);
    character->SetIsRunning(moveFromCollision);

    direction                     = camFront * direction.z + camRight * direction.x;

    const bool hasLookInput       = direction.LengthSq() > 0.1f * 0.1f;

    const float deltaTime         = AppEngine->GetGameTimer()->GetDeltaTime() / 1000.0f;
    const float playerSpeed       = character->GetSpeed();
    const float skinWidth         = 0.05f;
    const float lookAheadDistance = max(0.12f, playerSpeed * deltaTime);

    float3 lookDir                = direction;

    if (IsBlockedAhead(parent, direction, lookAheadDistance, skinWidth))
    {
        direction = float3::zero;
    }

    if (hasLookInput && !isAttacking) character->LookAt(lookDir);

    character->SetDirection(direction);

    // Heal
    if (keyboard[SDL_SCANCODE_E] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] == KEY_DOWN)
    {
        desiredTakeMushroom = true;
        takeMushroomCdTimer = takeMushroomCd;
    }
    if (keyboard[SDL_SCANCODE_R] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] == KEY_DOWN)
    {
        if (mushrooms != 0)
        {
            desiredHeal = true;
            healCdTimer = healCooldown;
        }
    }

    // Riastrad
    if (keyboard[SDL_SCANCODE_Q] == KEY_DOWN ||
        (input->GetLeftTrigger().first == KEY_REPEAT && input->GetRightTrigger().first == KEY_REPEAT))
    {
        if (!isRiastrad)
        {
            desiredTransform     = true;
            transformBufferTimer = inputBuffer;
        }
    }

    // Dash
    if (keyboard[SDL_SCANCODE_SPACE] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        desiredDash     = true;
        dashBufferTimer = inputBuffer;
    }

    // Attack
    if (mouse[SDL_BUTTON_LEFT - 1] == KEY_UP || controller[SDL_CONTROLLER_BUTTON_X] == KEY_UP)
    {
        desiredAttack     = true;
        attackBufferTimer = inputBuffer;
    }
    if (mouse[SDL_BUTTON_LEFT - 1] == KEY_REPEAT || controller[SDL_CONTROLLER_BUTTON_X] == KEY_REPEAT)
    {
        isChargingAttack = true;
    }
    if (mouse[SDL_BUTTON_LEFT - 1] == KEY_UP || controller[SDL_CONTROLLER_BUTTON_X] == KEY_UP)
    {
        isChargingAttack     = true;
        desiredChargedAttack = true;
    }

    // Ranged
    if (mouse[SDL_BUTTON_RIGHT - 1] == KEY_REPEAT || controller[SDL_CONTROLLER_BUTTON_Y] == KEY_REPEAT)
    {
        desiredAim = true;
    }
    if (input->GetLeftTrigger().first == KEY_UP)
    {
        if (state == CharacterStates::AIM) camera->EnableAimOffset(false);
    }
    if (mouse[SDL_BUTTON_RIGHT - 1] == KEY_UP || controller[SDL_CONTROLLER_BUTTON_Y] == KEY_UP)
    {
        if (state == CharacterStates::AIM && throwTimer <= 0.0f) ThrowSpear();
    }

    // Ultimatee
    if (keyboard[SDL_SCANCODE_F] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_B] == KEY_DOWN)
    {
        desiredUltimate     = true;
        ultimateBufferTimer = inputBuffer;
    }

    // Debug
    if (keyboard[SDL_SCANCODE_F5] == KEY_DOWN)
    {
        // TODO: This should be SetPosition, Respawn is here to test
        // SetPosition(spawnPos);
        Respawn();
    }
    if (keyboard[SDL_SCANCODE_F6] == KEY_DOWN)
    {
        spawnPos = parent->GetGlobalTransform().TranslatePart();
    }
    if (keyboard[SDL_SCANCODE_F7] == KEY_DOWN)
    {
        godMode = !godMode;
        if (godMode) GLOG("God Mode enabled")
        else GLOG("God Mode disabled")
    }
    if (keyboard[SDL_SCANCODE_F8] == KEY_DOWN)
    {
        AddRiastrad(100);
        GLOG("Fill riastrad")
    }
    if (keyboard[SDL_SCANCODE_F10] == KEY_DOWN)
    {
        AddRiastrad(10);
        Heal(10);
    }
    if (keyboard[SDL_SCANCODE_F9] == KEY_DOWN)
    {
        StartCurse();
    }

    // TODO: DELETE, JUST FOR TESTING FADEINOUT UI
    if (keyboard[SDL_SCANCODE_1] == KEY_DOWN)
    {
        const std::string testName = "FadeInOut";
        AppEngine->GetSceneModule()
            ->GetScene()
            ->GetGameObjectByName(testName)
            ->GetComponent<ShaderScriptComponent*>()
            ->GetScriptByType<UIFadeInOut>()
            ->FadeIn();
    }

    if (keyboard[SDL_SCANCODE_2] == KEY_DOWN)
    {
        const std::string testName = "FadeInOut";
        AppEngine->GetSceneModule()
            ->GetScene()
            ->GetGameObjectByName(testName)
            ->GetComponent<ShaderScriptComponent*>()
            ->GetScriptByType<UIFadeInOut>()
            ->FadeOut();
    }

    if (keyboard[SDL_SCANCODE_3] == KEY_DOWN)
    {
        const std::string testName = "FadeInOut";
        AppEngine->GetSceneModule()
            ->GetScene()
            ->GetGameObjectByName(testName)
            ->GetComponent<ShaderScriptComponent*>()
            ->GetScriptByType<UIFadeInOut>()
            ->Reset();
    }
}

bool CuChulainn::CanDash() const
{
    if (!dashUnlocked) return false; // When tutorial map is correctly fixed, put this to make progression

    bool canDash = dashTimer <= 0 && state != CharacterStates::AIM && !isAttacking && state != CharacterStates::FALL &&
                   state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE &&
                   state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::TAKE_MUSHROOM &&
                   state != CharacterStates::HEAL && !isCursed && state != CharacterStates::TRANSFORM &&
                   state != CharacterStates::HURT;

    if (canDash && state == CharacterStates::BASIC_ATTACK) canDash = comboBufferTimer > 0.0f;

    return canDash;
}

bool CuChulainn::CanAttack() const
{
    return attackPressTimer < chargeThreshold && state != CharacterStates::DASH && !isAttacking &&
           state != CharacterStates::FALL && state != CharacterStates::RESPAWN && comboCounter <= 1 &&
           attackCdTimer <= 0.0f && state != CharacterStates::ULTIMATE && state != CharacterStates::CHARGED_ATTACK &&
           state != CharacterStates::CHARGING && state != CharacterStates::TAKE_MUSHROOM &&
           state != CharacterStates::HEAL && state != CharacterStates::TRANSFORM && state != CharacterStates::HURT;
}

bool CuChulainn::CanUltimate() const
{
    bool canUltimate = state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
                       state != CharacterStates::RESPAWN && ultimateCdTimer <= 0.0f &&
                       state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::TAKE_MUSHROOM &&
                       state != CharacterStates::HEAL && state != CharacterStates::TRANSFORM &&
                       state != CharacterStates::HURT;

    if (canUltimate && state == CharacterStates::BASIC_ATTACK) canUltimate = comboBufferTimer > 0.0f;

    return canUltimate;
}

bool CuChulainn::CanTakeMushroom() const
{
    return state != CharacterStates::DASH && state != CharacterStates::BASIC_ATTACK && state != CharacterStates::AIM &&
           state != CharacterStates::RESPAWN && state != CharacterStates::DEATH && state != CharacterStates::FALL &&
           state != CharacterStates::ULTIMATE && state != CharacterStates::HEAL &&
           state != CharacterStates::TRANSFORM && state != CharacterStates::HURT;
}

bool CuChulainn::HasblockingTag(GameObject* go)
{
    if (!go) return false;
    for (const char* tagName : BlockerGOTags)
    {
        if (go->HasTag(HashString(tagName))) return true;
    }

    return false;
}

bool CuChulainn::CanHeal() const
{
    return state != CharacterStates::DASH && !isAttacking && state != CharacterStates::AIM &&
           state != CharacterStates::RESPAWN && state != CharacterStates::DEATH && state != CharacterStates::FALL &&
           state != CharacterStates::ULTIMATE && state != CharacterStates::TAKE_MUSHROOM &&
           state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::CHARGING && mushrooms > 0 &&
           !isHealing && state != CharacterStates::TRANSFORM && state != CharacterStates::HURT &&
           currentHealth < maxHealth;
}

bool CuChulainn::CanAim() const
{
    return state != CharacterStates::DASH && state != CharacterStates::BASIC_ATTACK && throwTimer <= 0 &&
           state != CharacterStates::FALL && state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE &&
           state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::CHARGING &&
           state != CharacterStates::TAKE_MUSHROOM && state != CharacterStates::HEAL &&
           state != CharacterStates::TRANSFORM && state != CharacterStates::HURT;
}

bool CuChulainn::CanChargeAttack() const
{
    bool canChargeAttack = state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
                           state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE &&
                           state != CharacterStates::AIM && state != CharacterStates::CHARGED_ATTACK &&
                           state != CharacterStates::TAKE_MUSHROOM && state != CharacterStates::HEAL &&
                           state != CharacterStates::TRANSFORM && state != CharacterStates::HURT;

    if (canChargeAttack && state == CharacterStates::BASIC_ATTACK) canChargeAttack = comboBufferTimer > 0.0f;

    return canChargeAttack;
}

bool CuChulainn::CanTransform() const
{
    bool canTransform = false;
    if (!isRiastrad)
    {
        canTransform = riastradMeter == 100 && state != CharacterStates::DASH && !isAttacking &&
                       character->IsGrounded() && state != CharacterStates::FALL && state != CharacterStates::RESPAWN &&
                       state != CharacterStates::ULTIMATE && state != CharacterStates::AIM &&
                       state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::TAKE_MUSHROOM &&
                       state != CharacterStates::HEAL && state != CharacterStates::TRANSFORM &&
                       state != CharacterStates::HURT;

        if (canTransform && state == CharacterStates::BASIC_ATTACK) canTransform = comboBufferTimer > 0.0f;
    }
    else
    {
        canTransform = state != CharacterStates::DASH && state != CharacterStates::BASIC_ATTACK &&
                       state != CharacterStates::FALL && state != CharacterStates::RESPAWN &&
                       state != CharacterStates::ULTIMATE && state != CharacterStates::AIM &&
                       state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::TAKE_MUSHROOM &&
                       state != CharacterStates::HEAL && state != CharacterStates::TRANSFORM;
    }

    return canTransform;
}

void CuChulainn::UpdateTimers(float deltaTime)
{
    weaponCollider->SetEnabled(false);
    Character::UpdateTimers(deltaTime);

    // Dash
    dashTimer -= deltaTime;
    if (dashTimer < 0.0f) dashTimer = 0.0f;
    if (desiredDash)
    {
        dashBufferTimer -= deltaTime;
        if (dashBufferTimer < 0.0f) desiredDash = false;
    }

    if (arrowVfxIsActive && arrowHitVfxObject && arrowHitVfxObject->IsEnabled())
    {
        arrowHitVfxTimer += deltaTime;
        if (arrowHitVfxTimer >= arrowHitVfxDuration)
        {
            arrowHitVfxObject->SetEnabled(false);
            arrowHitVfxTimer = 0.0f;
            arrowVfxIsActive = false;
        }
    }

    // Dash decal
    dashDecalBufferTimer -= deltaTime;
    if (dashDecalBufferTimer < 0.0f)
    {
        if (dashDecal) dashDecal->SetEnabled(false);

        dashDecalBufferTimer = 0.0f;
    }

    // Melee attack
    if (desiredAttack)
    {
        attackBufferTimer -= deltaTime;
        if (attackBufferTimer < 0.0f) desiredAttack = false;
    }

    // Ranged attack
    desiredAim  = false;
    throwTimer -= deltaTime;
    if (throwTimer < 0.0f)
    {
        if (resetWeapon)
        {
            weapon->SetEnabled(true);
            resetWeapon = false;
            spearCharacter->GetComponent<MeshComponent*>()->SetEnabled(true);
        }
        throwTimer = 0.0f;
    }

    // Take mushrooms
    takeMushroomCdTimer -= deltaTime;
    if (takeMushroomCdTimer <= 0.0f)
    {
        desiredTakeMushroom = false;
        takeMushroomCdTimer = 0.0f;
    }

    if (!isAttacking && comboBufferTimer > 0.0f)
    {
        comboBufferTimer -= deltaTime;
        if (comboBufferTimer <= 0.0f)
        {
            comboCounter  = -1;
            attackCdTimer = attackCooldown;
            if (state != CharacterStates::ULTIMATE && meleeTrailObject) meleeTrailObject->SetEnabled(false);

            if (state == CharacterStates::BASIC_ATTACK)
            {
                state = CharacterStates::IDLE;
                if (animComponent) animComponent->UseTrigger("AttackEnd");
            }
        }
    }

    // Ultimate
    ultimateCdTimer -= deltaTime;
    if (ultimateCdTimer < 0.0f) ultimateCdTimer = 0.0f;
    if (desiredUltimate)
    {
        ultimateBufferTimer -= deltaTime;
        if (ultimateBufferTimer < 0.0f) desiredUltimate = false;
    }

    // Charged attack
    if (isChargingAttack)
    {
        attackPressTimer += deltaTime;
        // GLOG("Attack press timer: %f", attackPressTimer);

        if (state == CharacterStates::CHARGING)
        {
            // GLOG("Charge timer: %f", chargeTimer);
            chargeTimer -= deltaTime;
            if (chargeTimer < 0.0f) chargeTimer = 0.0f;
        }
    }
    else
    {
        attackPressTimer = 0.0f;
    }
    isChargingAttack     = false;
    desiredChargedAttack = false;

    // Riastrad
    if (desiredTransform)
    {
        transformBufferTimer -= deltaTime;
        if (transformBufferTimer < 0.0f) desiredTransform = false;
    }
    if (isRiastrad)
    {
        riastradTimer -= deltaTime;
        if (riastradTimer <= 0.0f) desiredTransform = true;
    }

    if (isCursed)
    {
        curseTimer -= deltaTime;
        if (curseTimer <= 0.0f) EndCurse();
    }

    if (mushroomToEnable)
    {
        enableMushroomTimer -= deltaTime;
        if (enableMushroomTimer <= 0.0f && (hudMushrooms[mushrooms - 1]))
        {
            hudMushrooms[mushrooms - 1]->SetEnabled(true);
            mushroomToEnable = false;
        }
    }

    timeStopTimer -= AppEngine->GetGameTimer()->GetUnscaledDeltaTime() / 1000.0f;
    if (timeStopTimer <= 0.0f) AppEngine->GetGameTimer()->SetTimeScale(1.0f);

    const bool vfxUltimateActive = ultimateObject && ultimateObject->IsEnabled();
    if (state == CharacterStates::ULTIMATE || vfxUltimateActive) ultimateTimer += deltaTime;
    if (state == CharacterStates::CHARGED_ATTACK) chargedAttackTimer += deltaTime;
    if (state == CharacterStates::IDLE) idleTimer += deltaTime;
    if (state == CharacterStates::RUN) runTimer += deltaTime;
    if (state == CharacterStates::HEAL) healTimer += deltaTime;
    if (state == CharacterStates::TRANSFORM) transformTimer += deltaTime;

    if (state == CharacterStates::DASH || state == CharacterStates::HURT || state == CharacterStates::RESPAWN)
        isInvulnerable = true;

    isDashing = state == CharacterStates::DASH ? true : false;
    isHealing = state == CharacterStates::HEAL ? true : false;
}

void CuChulainn::LookAtMouse()
{
    const float3 mouseWorldPos = AppEngine->GetSceneModule()->GetScene()->GetMainCamera()->ScreenPointToXZ(
        parent->GetGlobalTransform().TranslatePart().y
    );
    float3 direction = mouseWorldPos - parent->GetGlobalTransform().TranslatePart();
    direction.y      = 0;
    direction.Normalize();
    character->LookAt(direction);
}

void CuChulainn::LookAtRightStick()
{
    const float2& stick    = AppEngine->GetInputModule()->GetRightStick();
    const float3 direction = camFront * stick.y + camRight * stick.x;
    if (direction.LengthSq() > 0.001f) character->LookAt(direction);
}

void CuChulainn::LookAtLeftStick()
{
    const float2& stick    = AppEngine->GetInputModule()->GetLeftStick();
    const float3 direction = camFront * stick.y + camRight * stick.x;
    if (direction.LengthSq() > 0.001f) character->LookAt(direction);
}

void CuChulainn::CheckIsFalling()
{
    const float verticalSpeed = character->GetRealSpeed().y;

    // GLOG("Vertical speed %f", verticalSpeed);
    if (verticalSpeed <= -1.0f && !character->IsGrounded() && animComponent)
    {
        animComponent->UseTrigger("Fall");
        state = CharacterStates::FALL;
    }

    if (state == CharacterStates::FALL && character->IsGrounded())
    {
        animComponent->UseTrigger("Land");
        character->EnableMovement(false);
    }

    const float maxDepth = -60.0f;

    if (parent->GetGlobalTransform().TranslatePart().y < maxDepth)
    {
        SetPosition(lastDashStartPos);
        TakeDamage(1);
    }
}

void CuChulainn::ThrowSpear()
{
    if (camera) camera->EnableAimOffset(false);
    if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_RANGEATTACK);
    if (animComponent) animComponent->UseTrigger("Ranged");
    aimTimer   = 0.0f;

    throwTimer = throwCooldown;
    if (weapon)
    {
        weapon->SetEnabled(false);
        resetWeapon = true;
        spearCharacter->GetComponent<MeshComponent*>()->SetEnabled(false);
    }
    if (aimShadowObject) aimShadowObject->SetEnabled(false);

    spear->Shoot(parent->GetGlobalTransform().TranslatePart(), character->GetFrontDirection());
}

void CuChulainn::Dash()
{
    if (state == CharacterStates::AIM && camera)
    {
        camera->EnableAimOffset(false);
        if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
    }
    else if (state == CharacterStates::BASIC_ATTACK)
    {
        comboBufferTimer = character->GetDashDuration() + 0.1f;
        isAttacking      = false;
    }
    else if (state == CharacterStates::CHARGING)
    {
        if (audio) audio->StopAudio();
    }

    desiredDash      = false;
    state            = CharacterStates::DASH;

    dashTimer        = isRiastrad ? dashCooldown * 0.75f : dashCooldown;
    lastDashStartPos = parent->GetGlobalTransform().TranslatePart();
    LookAtLeftStick();
    character->StartDash();
    isDashing = true;

    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_DASH);

    if (animComponent) animComponent->UseTrigger("Dash");
    if (dashTrail) dashTrail->SetEnabled(true);
    if (dashSmoke1)
    {
        const float3 characterPos = parent->GetGlobalTransform().TranslatePart();
        const float3 offset       = float3::unitY;

        const float3 scale        = dashSmoke1->GetParent()->GetLocalTransform().ExtractScale();
        const Quat lookRotation =
            Quat::LookAt(float3::unitZ, character->GetFrontDirection(), float3::unitY, float3::unitY);

        const Quat verticalCorrection   = Quat::RotateAxisAngle(float3::unitX, 90.0f * (PI / 180));
        const Quat horizontalCorrection = Quat::RotateAxisAngle(float3::unitZ, 90.0f * (PI / 180));

        const Quat finalRotation        = lookRotation * verticalCorrection * horizontalCorrection;

        const float4x4 transform        = float4x4::FromTRS(characterPos + offset, finalRotation, scale);

        const float4x4 parentWS         = parent->GetParentGlobalTransform();
        const float4x4 localTRS         = parentWS.Inverted() * transform;

        dashSmoke1->GetParent()->SetLocalTransform(localTRS);
        dashSmoke1->SetEnabled(true);
        dashSmoke1->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }
    if (dashSmoke2)
    {
        const float3 characterPos = parent->GetGlobalTransform().TranslatePart();
        const float3 offset       = float3::unitY * 0.1f;

        const float3 scale        = dashSmoke2->GetParent()->GetLocalTransform().ExtractScale();

        const Quat lookRotation =
            Quat::LookAt(float3::unitZ, character->GetFrontDirection(), float3::unitY, float3::unitY);

        const Quat finalRotation = lookRotation;

        const float4x4 transform = float4x4::FromTRS(characterPos + offset, finalRotation, scale);

        const float4x4 parentWS  = parent->GetParentGlobalTransform();
        const float4x4 localTRS  = parentWS.Inverted() * transform;

        dashSmoke2->GetParent()->SetLocalTransform(localTRS);
        dashSmoke2->SetEnabled(true);
        dashSmoke2->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }
}

void CuChulainn::PerformAttack()
{
    if (isAttacking && state == CharacterStates::BASIC_ATTACK)
    {
        float currentVfxDelay    = isRiastrad ? meleeVfxDelay / riastradAnimationsSpeedRatio : meleeVfxDelay;
        float currentHitboxDelay = isRiastrad ? attackHitboxDelay / riastradAnimationsSpeedRatio : attackHitboxDelay;
        float currentHitboxDuration =
            isRiastrad ? attackHitboxDuration / riastradAnimationsSpeedRatio : attackHitboxDuration;

        if (comboCounter == 2) currentHitboxDelay *= 1.5f;

        if (attackTimer > currentVfxDelay)
        {
            ShaderScriptComponent* vfxHorizontal = nullptr;
            ShaderScriptComponent* vfxVertical   = nullptr;
            switch (comboCounter)
            {
            case 0:
                vfxHorizontal = attackVfxHorizontal1;
                vfxVertical   = attackVfxVertical1;
                break;
            case 1:
                vfxHorizontal = attackVfxHorizontal2;
                vfxVertical   = attackVfxVertical2;
                break;
            case 2:
                vfxVertical = attackVfxVertical3;
                break;
            }

            if (vfxHorizontal && !vfxHorizontal->GetEnabled())
            {
                vfxHorizontal->SetEnabled(true);
                vfxHorizontal->GetScriptByType<AttackVfxSpritesheet>()->Reset();
            }
            if (vfxVertical && !vfxVertical->GetEnabled())
            {
                vfxVertical->SetEnabled(true);
                vfxVertical->GetScriptByType<AttackVfxSpritesheet>()->Reset();
            }
        }

        if (attackTimer < currentHitboxDelay)
        {
            float distance        = moveWithAttack ? 5.0f : 0.0f;
            float deltaTime       = AppEngine->GetGameTimer()->GetDeltaTime() / 1000.0f;

            float adaptedDistance = distance * deltaTime;
            const float skin      = 0.05f;

            if (!IsBlockedAhead(parent, character->GetFrontDirection(), max(0.55f, adaptedDistance), skin))
                character->MoveTo(distance);
        }
        else if (!weaponCollider->GetEnabled() && attackTimer >= currentHitboxDelay &&
                 attackTimer < currentHitboxDelay + currentHitboxDuration)
        {
            weaponCollider->SetEnabled(true);
            if (comboCounter == 2 && attackVfxExplosion && !attackVfxExplosion->GetEnabled())
            {
                attackVfxExplosion->SetEnabled(true);
                float3 dir = 2.0f * character->GetFrontDirection();
                dir.y      = 1.75f;
                attackVfxExplosion->GetParent()->SetLocalPosition(parent->GetPosition() + dir);
                attackVfxExplosion->GetScriptByType<AttackVfxSpritesheet>()->Reset();
            }
        }
        else if (weaponCollider->GetEnabled() && attackTimer >= currentHitboxDelay + currentHitboxDuration)
        {
            weaponCollider->SetEnabled(false);
        }
    }
    else if (state == CharacterStates::ULTIMATE)
    {
        if (!ultimateSoundPlayed && audio && ultimateTimer >= 0.55f)
        {
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_ULTIMATEATTACK);
            ultimateSoundPlayed = true;
        }

        float currentHitboxDelay =
            isRiastrad ? ultimateHitboxDelay / riastradAnimationsSpeedRatio : ultimateHitboxDelay;
        float currentHitboxDuration =
            isRiastrad ? ultimateHitboxDuration / riastradAnimationsSpeedRatio : ultimateHitboxDuration;
        float currentAnimationDelay =
            isRiastrad ? ultimateAnimationDelay / riastradAnimationsSpeedRatio : ultimateAnimationDelay;

        if (!ultimateObject->IsEnabled() && ultimateTimer >= currentAnimationDelay)
        {
            ultimateObject->GetComponent<AnimationComponent*>()->SetDefaultPlaybackSpeed(ultimateSpeed);
            ultimateObject->GetComponent<AnimationComponent*>()->OnStop();
            ultimateObject->GetComponent<AnimationComponent*>()->OnPlay(false, false);
            ultimateObject->GetComponent<AnimationComponent*>()->GetAnimationController()->SetTime(0.0f);
            ultimateObject->SetEnabled(true);
            UpdateUltimateVfx();
            ultimateObject->GetComponent<AnimationComponent*>()->Update(0.0f);
            ultimateObject->GetComponent<SphereColliderComponent*>()->SetEnabled(false);

            if (ultimateHoldEnabled && animComponent && !playerAnimHeld)
            {
                animComponent->OnPause();
                playerAnimHeld = true;
            }
        }
        else if (ultimateObject->IsEnabled())
        {
            AnimationComponent* vfxUltimateAnim  = ultimateObject->GetComponent<AnimationComponent*>();
            vfxTimeUnscaledSec                  += AppEngine->GetGameTimer()->GetUnscaledDeltaTime() / 1000.0f;

            if (ultimateHoldEnabled && playerAnimHeld) // control cuchulainn stop animation
            {
                float timeLimit = (vfxUltimateAnim && vfxUltimateAnim->GetAnimationController())
                                    ? vfxUltimateAnim->GetAnimationController()->GetTime()
                                    : 0.0f;

                if (!vfxUltimateAnim || vfxUltimateAnim->IsFinished() ||
                    timeLimit >= ultimateResumeVfxTime / ultimateSpeed)
                {
                    animComponent->OnResume();
                    playerAnimHeld = false;
                }
            }

            if (ultimateSpikes) // Control spikes animation appearance
            {
                const bool animReady =
                    vfxUltimateAnim && vfxUltimateAnim->GetCurrentAnimation() && !vfxUltimateAnim->IsFinished();
                bool show, blurShow, warningShow = false;

                if (animReady)
                {
                    const float vfxLenAnim    = vfxUltimateAnim->GetCurrentAnimation()->GetDuration();

                    const float spikesOff     = min(2.12f, vfxLenAnim - 0.05f) / ultimateSpeed;
                    const float vfxLocalTimer = vfxTimeUnscaledSec;

                    show                      = (vfxLocalTimer >= 0.40f / ultimateSpeed) && (vfxLocalTimer < spikesOff);
                    blurShow                  = vfxLocalTimer >= 0.19f / ultimateSpeed;
                    warningShow               = vfxLocalTimer <= 0.4f / ultimateSpeed;
                }

                if (ultimateSpikes->IsEnabled() != show) ultimateSpikes->SetEnabled(show);
                if (ultimateCrack && ultimateCrack->IsEnabled() != show) ultimateCrack->SetEnabled(show);
                if (blurShow) ultimateBlur->GetComponent<ShaderScriptComponent*>()->SetEnabled(true);
                if (!warningShow) ultimateWarning->SetEnabled(false);
            }
            if (ultimateTimer >= currentHitboxDelay + currentAnimationDelay &&
                ultimateTimer < currentHitboxDelay + currentHitboxDuration + currentAnimationDelay)
            {
                ultimateObject->GetComponent<SphereColliderComponent*>()->SetEnabled(true);
            }
            else
            {
                ultimateObject->GetComponent<SphereColliderComponent*>()->SetEnabled(false);
            }
        }
    }
    else if (state == CharacterStates::CHARGED_ATTACK)
    {
        float currentHitboxDelay =
            isRiastrad ? chargedAttackHitboxDelay / riastradAnimationsSpeedRatio : chargedAttackHitboxDelay;
        float currentHitboxDuration =
            isRiastrad ? chargedAttackHitboxDuration / riastradAnimationsSpeedRatio : chargedAttackHitboxDuration;

        if (!chargedAttackCollider->IsEnabled() && chargedAttackTimer >= currentHitboxDelay &&
            chargedAttackTimer < currentHitboxDelay + currentHitboxDuration)
        {
            chargedAttackCollider->SetEnabled(true);
        }
        else if (chargedAttackCollider->IsEnabled() && chargedAttackTimer >= currentHitboxDelay + currentHitboxDuration)
        {
            chargedAttackCollider->SetEnabled(false);
        }
    }
}

void CuChulainn::Attack(float deltaTime)
{
    if (state == CharacterStates::AIM && camera)
    {
        if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
        camera->EnableAimOffset(false);
    }

    ++comboCounter;
    desiredAttack = false;
    state         = CharacterStates::BASIC_ATTACK;
    character->EnableMovement(false);

    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_NORMALATTACK_01);
    if (meleeTrailObject) meleeTrailObject->SetEnabled(true);

    Character::Attack(deltaTime);
    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    else LookAtLeftStick();
    if (animComponent)
    {
        const std::string trigger = "Attack" + std::to_string(comboCounter);
        animComponent->UseTrigger(trigger);
    }

    // Raycast to check if enemy in front, to decide wether to move forward or not
    const float3 position         = parent->GetGlobalTransform().TranslatePart();
    const float3 direction        = character->GetFrontDirection();
    const float3 lateralDirection = direction.Cross(float3::unitY).Normalized();

    const float3 rightRayOrigin   = position + lateralDirection * 0.3f;
    const float3 rightRayOrigin2  = position + lateralDirection * 0.6f;
    const float3 rightRayOrigin3  = position + lateralDirection * 0.9f;

    const float3 leftRayOrigin    = position - lateralDirection * 0.3f;
    const float3 leftRayOrigin2   = position - lateralDirection * 0.6f;
    const float3 leftRayOrigin3   = position - lateralDirection * 0.9f;

    LineSegment centerRay(position + direction * 0.075f, position + direction * 3.0f);

    LineSegment leftRay(leftRayOrigin, leftRayOrigin + direction * 3.0f);
    LineSegment leftRay2(leftRayOrigin2 - direction * 0.2f, leftRayOrigin2 + direction * 3.0f);
    LineSegment leftRay3(leftRayOrigin3 - direction * 0.2f, leftRayOrigin3 + direction * 3.0f);

    LineSegment rightRay(rightRayOrigin, rightRayOrigin + direction * 3.0f);
    LineSegment rightRay2(rightRayOrigin2 - direction * 0.2f, rightRayOrigin2 + direction * 3.0f);
    LineSegment rightRay3(rightRayOrigin3 - direction * 0.2f, rightRayOrigin3 + direction * 3.0f);

    BulletUserPointer* centerHit = RaycastController::GetRayIntersectionPhysics(centerRay);

    BulletUserPointer* leftHit   = RaycastController::GetRayIntersectionPhysics(leftRay);
    BulletUserPointer* leftHit2  = RaycastController::GetRayIntersectionPhysics(leftRay2);
    BulletUserPointer* leftHit3  = RaycastController::GetRayIntersectionPhysics(leftRay3);

    BulletUserPointer* rightHit  = RaycastController::GetRayIntersectionPhysics(rightRay);
    BulletUserPointer* rightHit2 = RaycastController::GetRayIntersectionPhysics(rightRay2);
    BulletUserPointer* rightHit3 = RaycastController::GetRayIntersectionPhysics(rightRay3);

    if (centerHit || leftHit || leftHit2 || leftHit3 || rightHit || rightHit2 || rightHit3)
    {
        moveWithAttack = false;
    }
    else
    {
        moveWithAttack = true;
    }

    DebugDrawModule* debug = AppEngine->GetDebugDrawModule();
    if (debug->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        float3 centralColor = centerHit != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(centerRay, centralColor);

        float3 rightColor = rightHit != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(rightRay, rightColor);
        float3 rightColor2 = rightHit2 != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(rightRay2, rightColor2);
        float3 rightColor3 = rightHit3 != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(rightRay3, rightColor3);

        float3 leftColor = leftHit != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(leftRay, leftColor);
        float3 leftColor2 = leftHit2 != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(leftRay2, leftColor2);
        float3 leftColor3 = leftHit3 != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
        debug->DrawLineSegment(leftRay3, leftColor3);
    }
}

void CuChulainn::UltimateAttack()
{
    // GLOG("ULTIMATEEEE");
    if (ultimateSpikes) ultimateSpikes->SetEnabled(false);
    if (ultimateCrack) ultimateCrack->SetEnabled(false);

    if (state == CharacterStates::AIM && camera)
    {
        camera->EnableAimOffset(false);
        if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
    }
    state = CharacterStates::ULTIMATE;
    character->EnableMovement(false);
    ultimateTimer       = 0.0f;
    ultimateCdTimer     = ultimateCd;
    desiredUltimate     = false;
    playerAnimHeld      = false;
    controlsLocked      = true;
    ultimateSoundPlayed = false;

    if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
    // if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_ULTIMATEATTACK);
    if (animComponent) animComponent->UseTrigger("Ultimate");
}

void CuChulainn::UpdateUltimateVfx()
{
    if (ultimateBlur)
    {
        ultimateBlur->SetEnabled(true);
        if (ultimateBlur->GetComponent<ShaderScriptComponent*>())
        {
            ultimateBlur->GetComponent<MeshComponent*>()->SetEnabled(false);
            ultimateBlur->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
            ultimateBlur->GetComponent<ShaderScriptComponent*>()->SetEnabled(false);
        }
    }
    if (ultimateBrust)
    {
        ultimateBrust->SetEnabled(true);
        if (ultimateBrust->GetComponent<ShaderScriptComponent*>())
        {
            ultimateBrust->GetComponent<MeshComponent*>()->SetEnabled(false);
            ultimateBrust->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
        }
    }
    if (ultimateCrack)
    {
        ultimateCrack->SetEnabled(true);
        if (ultimateCrack->GetComponent<ShaderScriptComponent*>())
        {
            ultimateCrack->GetComponent<MeshComponent*>()->SetEnabled(false);
            ultimateCrack->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
        }
    }
    if (ultimateWarning)
    {
        ultimateWarning->SetEnabled(true);
        if (ultimateWarning->GetComponent<ShaderScriptComponent*>())
        {
            ultimateWarning->GetComponent<MeshComponent*>()->SetEnabled(false);
            ultimateWarning->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
        }
    }
    if (ultimateCrack) ultimateCrack->SetEnabled(false);
    if (ultimateSpikes) ultimateSpikes->SetEnabled(false);
    vfxTimeUnscaledSec = 0.0f;
}

void CuChulainn::Aim(float deltaTime)
{
    if (!spear) return;

    if (state != CharacterStates::AIM)
    {
        if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
        if (camera) camera->EnableAimOffset(true);
        state = CharacterStates::AIM;
        character->EnableMovement(false);
        if (animComponent) animComponent->UseTrigger("Ranged");
        if (aimShadowObject) aimShadowObject->SetEnabled(true);
    }
    desiredAim = false;

    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    else LookAtLeftStick();
}

void CuChulainn::Move()
{
    character->EnableMovement(true);

    const bool actuallyMoving = character->IsMoving();
    const bool wantsMove      = moveFromCollision;
    const bool runCondition   = wantsMove || character->GetSpeed() > 0.5f;

    if (runCondition)
    {
        if (state != CharacterStates::RUN)
        {
            state    = CharacterStates::RUN;
            runTimer = 0.0f;
            if (animComponent) animComponent->UseTrigger("Walk");
        }

        if (runTimer > stepTime && audio)
        {
            LineSegment ray(
                parent->GetGlobalTransform().TranslatePart(),
                parent->GetGlobalTransform().TranslatePart() - float3::unitY
            );
            GameObject* object = RaycastController::GetRayIntersectionTrees(
                ray, AppEngine->GetSceneModule()->GetScene()->GetOctree(),
                AppEngine->GetSceneModule()->GetScene()->GetDynamicTree()
            );

            if (object)
            {
                // Default to grass steps
                AkUniqueID eventId = AK::EVENTS::PLAY_SFX_STEPS_GRASS;
                if (object->HasTag(HashString("Wood"))) eventId = AK::EVENTS::PLAY_SFX_STEPS_WOOD;
                else if (object->HasTag(HashString("Rock"))) eventId = AK::EVENTS::PLAY_SFX_STEPS_ROCK;

                if (audio) audio->EmitEvent(eventId);
            }

            runTimer = 0.0f;
        }
    }
    else
    {
        if (state != CharacterStates::IDLE)
        {
            if (animComponent) animComponent->UseTrigger("Idle");
            state     = CharacterStates::IDLE;
            idleTimer = 0.0f;
        }

        if (idleTimer > 8.0f && animComponent)
        {
            float x = (float)rand() / RAND_MAX;
            if (x < 0.5f) animComponent->UseTrigger("IdleBreak1");
            else animComponent->UseTrigger("IdleBreak2");
            idleTimer = 0.0f;
        }
    }
}

void CuChulainn::SetPosition(const float3& position)
{
    parent->SetLocalPosition(position - parent->GetParentGlobalTransform().TranslatePart());
    if (camera) camera->SetPosition(position - parent->GetParentGlobalTransform().TranslatePart());
}

void CuChulainn::Respawn()
{
    GLOG("[PLAYER] Respawn()");
    Character::Restart();

    isDead         = false;
    deathTimer     = 0.0f;

    currentHealth  = maxHealth;
    reservedHealth = maxHealth;
    state          = CharacterStates::RESPAWN;

    if (healthBar) healthBar->SetFillAmount(static_cast<float>(currentHealth) / static_cast<float>(maxHealth));
    if (damageMask) damageMask->SetLife(static_cast<float>(currentHealth));

    SetPosition(spawnPos);
    if (animComponent) animComponent->UseTrigger("Respawn");
    character->EnableMovement(false);

    GLOG("[PLAYER] -> state=RESPAWN hp=%d", currentHealth);
}

void CuChulainn::TakeDamage(int amount)
{
    int prev = currentHealth;
    GLOG("[PLAYER] TakeDamage(%d) hpBefore=%d state=%s", amount, prev, GetLogicStateName().c_str());
    if (godMode || isRiastrad || state == CharacterStates::ULTIMATE)
    {
        GLOG("[PLAYER] TakeDamage -> INVULNERABLE (ignorat)");
        return;
    }
    Character::TakeDamage(amount);
}

bool CuChulainn::TakeMushroom()
{
    bool taken = false;
    if (mushrooms <= 2)
    {
        mushroomToEnable    = true;
        enableMushroomTimer = 0.25f;
        if (hudMushroomsPick[mushrooms])
        {
            hudMushroomsPick[mushrooms]->SetEnabled(true);
            hudMushroomsPick[mushrooms]->GetScriptByType<UISpritesheet>()->Reset();
        }

        state      = CharacterStates::TAKE_MUSHROOM;
        taken      = true;

        mushrooms += 1;

        if (animComponent) animComponent->UseTrigger("Pick");
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_LIFEMUSHROOMS);
        character->EnableMovement(false);
    }

    desiredTakeMushroom = false;

    return taken;
}

void CuChulainn::UseMushroom()
{
    mushrooms   -= 1;

    state        = CharacterStates::HEAL;
    desiredHeal  = false;

    if (animComponent) animComponent->UseTrigger("Heal");

    character->EnableMovement(false);
    isHealing = true;

    if (hudMushrooms[mushrooms]) hudMushrooms[mushrooms]->SetEnabled(false);
    if (hudMushroomsPick[mushrooms])
    {
        hudMushroomsUse[mushrooms]->SetEnabled(true);
        hudMushroomsUse[mushrooms]->GetScriptByType<UISpritesheet>()->Reset();
    }

    if (healVfx)
    {
        healVfx->SetEnabled(true);
        if (healParticles) healParticles->SetEnabled(false);
        healVfx->SetLocalPosition(parent->GetLocalTransform().TranslatePart());
        Scene* scene = AppEngine->GetSceneModule()->GetScene();
        for (UID child : healVfx->GetChildren())
        {
            GameObject* currentChild = scene->GetGameObjectByUID(child);
            MeshComponent* mesh      = currentChild->GetComponent<MeshComponent*>();
            if (mesh) mesh->SetEnabled(false);
            ShaderScriptComponent* shaderScript = currentChild->GetComponent<ShaderScriptComponent*>();
            if (shaderScript)
            {
                for (Script* script : shaderScript->GetScriptInstances())
                {
                    script->Reset();
                }
            }
        }
    }

    healTimer = 0.0f;
}

void CuChulainn::ChargeAttack()
{
    if (state != CharacterStates::CHARGING)
    {
        if (chargeVfx1)
        {
            chargeVfx1->SetEnabled(true);
            chargeVfx1->GetScriptByType<AttackVfxSpritesheet>()->Reset();
        }
        if (chargeVfx2)
        {
            chargeVfx2->SetEnabled(true);
            chargeVfx2->GetScriptByType<AttackVfxSpritesheet>()->Reset();
        }
        if (chargeVfx3)
        {
            chargeVfx3->SetEnabled(true);
            chargeVfx3->GetScriptByType<AttackVfxSpritesheet>()->Reset();
        }

        state       = CharacterStates::CHARGING;
        chargeTimer = isRiastrad ? chargeDuration * 0.5f : chargeDuration;
        character->EnableMovement(false);

        if (animComponent) animComponent->UseTrigger("Charge");
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_CHARGEDATTACKSTART);
    }
    else if (desiredChargedAttack)
    {
        desiredChargedAttack = false;
        isChargingAttack     = false;

        if (chargeVfx1) chargeVfx1->SetEnabled(false);
        if (chargeVfx2) chargeVfx2->SetEnabled(false);
        if (chargeVfx3) chargeVfx3->SetEnabled(false);

        if (chargeTimer <= 0.0f)
        {
            state              = CharacterStates::CHARGED_ATTACK;
            chargedAttackTimer = 0.0f;
            if (meleeTrailObject) meleeTrailObject->SetEnabled(true);

            if (animComponent) animComponent->UseTrigger("Attack");
            if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_CHARGEDATTACK);

            if (chargedAttackVfx)
            {
                chargedAttackVfx->SetEnabled(true);
                chargedAttackVfx->GetScriptByType<AttackVfxSpritesheet>()->Reset();
            }
        }
        else
        {
            GLOG("NOT CHARGED ENOUFGH");
            character->EnableMovement(true);
            state = CharacterStates::IDLE;
            if (animComponent) animComponent->UseTrigger("Idle");
            if (audio) audio->StopAudio();
        }
    }
}

void CuChulainn::ToggleRiastrad()
{
    desiredTransform = false;

    if (!isRiastrad)
    {
        EndCurse();

        // Start Riastrad
        AddRiastrad(-100);
        isRiastrad    = true;
        riastradTimer = riastradDuration;
        riastradMeter = 0;
        character->SetMaxSpeed(riastradMovementSpeed);
        Heal(maxHealth);

        state = CharacterStates::TRANSFORM;
        character->EnableMovement(false);

        if (animComponent) animComponent->UseTrigger("Transform");

        const HashString idleName = HashString("Idle");
        const HashString walkName = HashString("Walk");
        for (State& state : animComponent->GetResourceStateMachine()->states)
        {
            if (state.name == idleName)
            {
                if (rand() % 2 == 0) state.clipName = riastradIdleName;
                else state.clipName = riastradIdleName2;
            }
            else if (state.name == walkName)
            {
                state.clipName = riastradRunName;
                for (Clip& clip : animComponent->GetResourceStateMachine()->clips)
                {
                    if (clip.clipName == state.clipName) clip.animationSpeed = 1.5f;
                }
            }
        }

        if (riastradVfx)
        {
            riastradVfx->GetComponent<AnimationComponent*>()->OnPlay(true);
            riastradVfx->SetLocalPosition(parent->GetLocalTransform().TranslatePart());
        }

        if (riastradVfxBG)
        {
            riastradVfxBG->SetEnabled(true);
            riastradVfxBG->GetScriptByType<UISpritesheet>()->Reset();
        }
        if (riastradVfxFG)
        {
            riastradVfxFG->SetEnabled(true);
            riastradVfxFG->GetScriptByType<UISpritesheet>()->Reset();
        }

        riastradKey->SetEnabled(false);
        riastradTriggers->SetEnabled(false);

        if (audio)
        {
            audio->EmitEvent(AK::EVENTS::SET_GAMESTATE_RIASTRAD);
            audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_TRANSFORM);
        }
    }
    else
    {
        // Stop Riastrad
        isRiastrad = false;
        character->SetMaxSpeed(defaultSpeed);

        const HashString idleName = HashString("Idle");
        const HashString walkName = HashString("Walk");
        for (State& state : animComponent->GetResourceStateMachine()->states)
        {
            if (state.name == idleName)
            {
                state.clipName = defaultIdleName;
            }
            else if (state.name == walkName)
            {
                state.clipName = defaultRunName;
                for (Clip& clip : animComponent->GetResourceStateMachine()->clips)
                {
                    if (clip.clipName == state.clipName) clip.animationSpeed = 1.0f;
                }
            }
        }

        if (riastradEye)
        {
            riastradEye->SetFillAmount(0);

            if (riastradVfxFG) riastradVfxFG->SetEnabled(false);
            if (riastradVfxBG) riastradVfxBG->SetEnabled(false);
            if (riastradFireUp) riastradFireUp->SetEnabled(false);
            if (riastradFireDown) riastradFireDown->SetEnabled(false);
        }

        if (animComponent) animComponent->UseTrigger("Idle");
        state         = CharacterStates::IDLE;

        // TODO: Remove when VFX
        Resource* res = AppEngine->GetResourcesModule()->RequestResource(playerMaterial);
        if (res)
        {
            ResourceMaterial* mat = static_cast<ResourceMaterial*>(res);
            float4 newColor       = mat->GetMaterial().diffColor;
            newColor              = float4::one;
            mat->SetDiffColor(newColor);
        }

        GameObject* musicManager = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("MusicManager");
        if (musicManager != nullptr)
        {
            musicManager->GetComponent<ScriptComponent*>()->GetScriptByType<MusicManager>()->ResetToCachedGameState();
        }
    }
}

void CuChulainn::EnableRiastradVfx()
{
    if (transformTimer < transformVfxDelay) return;

    // Reuse charge attack collider (If needed different size, create another)
    chargedAttackCollider->SetEnabled(true);

    // TODO: Remove when VFX. For now it turns red
    Resource* res = AppEngine->GetResourcesModule()->RequestResource(playerMaterial);
    if (res)
    {
        ResourceMaterial* mat = static_cast<ResourceMaterial*>(res);
        float4 newColor       = mat->GetMaterial().diffColor;
        newColor.y            = 0.0f;
        newColor.z            = 0.0f;
        mat->SetDiffColor(newColor);
    }

    if (riastradCrack)
    {
        riastradCrack->SetEnabled(true);
        riastradCrack->GetComponent<MeshComponent*>()->SetEnabled(false);
        riastradCrack->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
    }
    if (riastradWarning)
    {
        riastradWarning->SetEnabled(true);
        riastradWarning->GetComponent<MeshComponent*>()->SetEnabled(false);
        riastradWarning->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
    }
    if (riastradBlur)
    {
        riastradBlur->SetEnabled(true);
        riastradBlur->GetComponent<MeshComponent*>()->SetEnabled(false);
        riastradBlur->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
    }
    if (riastradStars)
    {
        riastradStars->SetEnabled(true);
        riastradStars->GetComponent<MeshComponent*>()->SetEnabled(false);
        riastradStars->GetComponent<ShaderScriptComponent*>()->GetScriptByType<MovingUVTransparent>()->Reset();
    }

    if (riastradFireUp)
    {
        riastradFireUp->SetEnabled(true);
        riastradFireUp->GetScriptByType<UISpritesheet>()->Reset();
    }
    if (riastradFireDown)
    {
        riastradFireDown->SetEnabled(true);
        riastradFireDown->GetScriptByType<UISpritesheet>()->Reset();
    }

    if (riastradSmoke)
    {
        const float3 characterPos =
            parent->GetGlobalTransform().TranslatePart() - parent->GetParentGlobalTransform().TranslatePart();

        const float3 offset = float3(-2.0f, 4.0f, 1.5f);

        riastradSmoke->GetParent()->SetLocalPosition(characterPos + offset);
        riastradSmoke->SetEnabled(true);
        riastradSmoke->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }

    if (riastradGroundExplosion)
    {
        riastradGroundExplosion->SetEnabled(true);
        riastradGroundExplosion->GetScriptByType<AttackVfxSpritesheet>()->Reset();
    }
}

void CuChulainn::AddRiastrad(int amount)
{
    riastradMeter += amount;
    if (riastradMeter > 100) riastradMeter = 100;

    if (riastradMeter == 100)
    {
        if (riastradEye) riastradEye->SetFillAmount(riastradMeter / 100.0f);
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_MC_RIASTRADCHARGED);

        if (AppEngine->GetInputModule()->IsUsingKeyboard() && riastradKey) riastradKey->SetEnabled(true);
        else if (riastradTriggers) riastradTriggers->SetEnabled(true);
    }

    if (!riastradBar) return;
    riastradBar->SetFillAmount(riastradMeter / 100.0f);
}

void CuChulainn::OnObjectDestroyed()
{
    AddRiastrad(riastradOnObjectHit);
}

void CuChulainn::OnEnemyHit()
{
    AppEngine->GetGameTimer()->SetTimeScale(0.0f);
    timeStopTimer = hitTimeStopDuration;
    AddRiastrad(riastradOnEnemyHit);
}

void CuChulainn::OnEnemyDefeated()
{
    AppEngine->GetGameTimer()->SetTimeScale(0.0f);
    timeStopTimer = deathTimeStopDuration;
    AddRiastrad(riastradOnEnemyDeath);
}

void CuChulainn::ActivateAbility(std::string abilityName)
{
    std::transform(abilityName.begin(), abilityName.end(), abilityName.begin(), ::tolower);

    if (abilityName == "dash") dashUnlocked = true;
    else if (abilityName == "ultimate") ultimateUnlocked = true;
}

void CuChulainn::OnArrowHit()
{
    arrowVfxIsActive = true;
}

void CuChulainn::StartCurse()
{
    // TODO: Remove when VFX
    Resource* res = AppEngine->GetResourcesModule()->RequestResource(playerMaterial);
    if (res)
    {
        ResourceMaterial* mat = static_cast<ResourceMaterial*>(res);
        float4 newColor       = mat->GetMaterial().diffColor;
        newColor.y            = 0.0f;
        newColor.x            = 0.6f;
        mat->SetDiffColor(newColor);
    }

    isCursed = true;
    character->SetMaxSpeed(curseSpeed);
    curseTimer                = curseDuration;

    const HashString walkName = HashString("Walk");
    for (State& state : animComponent->GetResourceStateMachine()->states)
    {
        if (state.name == walkName)
        {
            state.clipName = curseRunName;
            for (Clip& clip : animComponent->GetResourceStateMachine()->clips)
            {
                if (clip.clipName == state.clipName) clip.animationSpeed = 3.0f;
            }
        }
    }

    if (animComponent) animComponent->UseTrigger("Idle");
    state = CharacterStates::IDLE;
}
void CuChulainn::ExportState(PlayerState& playerState) const
{
    playerState.currentHealth    = currentHealth;
    playerState.maxHealth        = maxHealth;
    playerState.riastrad         = riastradMeter;
    playerState.mushrooms        = mushrooms;
    playerState.dashUnlocked     = dashUnlocked;
    playerState.ultimateUnlocked = ultimateUnlocked;
}

void CuChulainn::ApplySavedState(const PlayerState& playerState)
{
    maxHealth      = max(5, playerState.maxHealth);
    currentHealth  = std::clamp(playerState.currentHealth, 1, maxHealth);
    reservedHealth = currentHealth;

    if (healthBar) healthBar->SetFillAmount(static_cast<float>(currentHealth) / static_cast<float>(maxHealth));
    if (damageMask) damageMask->SetLife(static_cast<float>(currentHealth));

    riastradMeter = 0;
    AddRiastrad(playerState.riastrad);

    mushrooms = playerState.mushrooms;

    if (playerState.dashUnlocked) ActivateAbility(static_cast<std::string>("dash"));
    if (playerState.ultimateUnlocked) ActivateAbility(static_cast<std::string>("ultimate"));
}

void CuChulainn::EndCurse()
{
    // TODO: Remove when VFX
    Resource* res = AppEngine->GetResourcesModule()->RequestResource(playerMaterial);
    if (res)
    {
        ResourceMaterial* mat = static_cast<ResourceMaterial*>(res);
        float4 newColor       = mat->GetMaterial().diffColor;
        newColor              = float4::one;
        mat->SetDiffColor(newColor);
    }

    isCursed = false;
    character->SetMaxSpeed(defaultSpeed);

    const HashString walkName = HashString("Walk");
    for (State& state : animComponent->GetResourceStateMachine()->states)
    {
        if (state.name == walkName)
        {
            state.clipName = defaultRunName;
            for (Clip& clip : animComponent->GetResourceStateMachine()->clips)
            {
                if (clip.clipName == state.clipName) clip.animationSpeed = 1.0f;
            }
        }
    }

    if (animComponent) animComponent->UseTrigger("Idle");
    state = CharacterStates::IDLE;
}

bool CuChulainn::IsBlockedAhead(
    const GameObject* ownerGO, const float3& desiredMoveDirection, float lookAheadDistance, float skinWidth
)
{
    if (!ownerGO || desiredMoveDirection.LengthSq() < 0.001f) return false;

    Scene* currentScene              = AppEngine->GetSceneModule()->GetScene();
    const float3 playerWorldPosition = ownerGO->GetGlobalTransform().TranslatePart();
    const float3 normMoveDir         = desiredMoveDirection.Normalized();

    auto hitsBlockAtHeight           = [&](float height)
    {
        const float3 rayStart = playerWorldPosition + float3::unitY * height;

        LineSegment ray(rayStart, rayStart + normMoveDir * (lookAheadDistance + skinWidth));

        GameObject* hitGO              = nullptr;
        BulletUserPointer* userPointer = RaycastController::GetRayIntersectionPhysics(ray);
        if (userPointer)
        {
            hitGO = userPointer->collider->GetParent();
            GLOG("[IsBlockedAhead]: Physics Raycast hit!, %s", hitGO->GetName().c_str());
        }

        DebugDrawModule* debug = AppEngine->GetDebugDrawModule();

        if (debug->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
        {
            float3 centralColor = hitGO != nullptr ? float3(1.0f, 0.0f, 0.0f) : float3(1.0f, 1.0f, 0.0f);
            debug->DrawLineSegment(ray, centralColor);
        }
        return hitGO && HasblockingTag(hitGO);
    };

    return hitsBlockAtHeight(0.2f) || hitsBlockAtHeight(0.9f);
}

const std::string CuChulainn::GetLogicStateName()
{
    switch (state)
    {
    case CharacterStates::IDLE:
        return "Idle";
        break;
    case CharacterStates::RUN:
        return "Run";
        break;
    case CharacterStates::DASH:
        return "Dash";
        break;
    case CharacterStates::BASIC_ATTACK:
        return "Basic Attack";
        break;
    case CharacterStates::AIM:
        return "Aiming";
        break;
    case CharacterStates::RESPAWN:
        return "Respawn";
        break;
    case CharacterStates::DEATH:
        return "Death";
        break;
    case CharacterStates::FALL:
        return "Falling";
        break;
    case CharacterStates::ULTIMATE:
        return "Ultimate";
        break;
    case CharacterStates::CHARGING:
        return "Charging";
        break;
    case CharacterStates::CHARGED_ATTACK:
        return "Charged Attack";
        break;
    case CharacterStates::TAKE_MUSHROOM:
        return "Take Mushroom";
        break;
    case CharacterStates::HEAL:
        return "Healing";
        break;
    case CharacterStates::TRANSFORM:
        return "Transform";
        break;
    case CharacterStates::HURT:
        return "Hurt";
    default:
        return "MISSING!";
        break;
    }
}

bool CuChulainn::ConsumeJustDied()
{
    if (justDied)
    {
        justDied = false; // reset the flag after consuming
        return true;
    }
    return false; // nothing to consume
}

bool CuChulainn::IsGameOverCondition() const
{
    return isDead || state == CharacterStates::DEATH || currentHealth <= 0;
}
