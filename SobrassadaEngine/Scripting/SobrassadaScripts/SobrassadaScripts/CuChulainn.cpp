#include "pch.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraMovement.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "Projectile.h"
#include "ResourceStateMachine.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Standalone/UI/ImageComponent.h"

#include "Math/Quat.h"
#include "SDL.h"
#include "Wwise_IDs.h"

CharacterControllerComponent* character = nullptr;
CuChulainn* playerScript                = nullptr;

CuChulainn::CuChulainn(GameObject* parent)
    : Character(parent, 5, 1, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, CharacterType::CuChulainn)
{
    currentHealth = 3; // mainChar starts low hp

    // TODO: Replace target names by gameObjects when overriding prefabs doesn't break the link
    fields.push_back({"Camera Object Name", InspectorField::FieldType::InputText, &cameraName});
    fields.push_back({"Spear Projectile Name", InspectorField::FieldType::InputText, &spearName});
    fields.push_back({"Range attack cooldown", InspectorField::FieldType::Float, &throwCooldown, 0.0f, 2.0f});
    fields.push_back({"Dash cooldown", InspectorField::FieldType::Float, &dashCooldown, 0.0f, 5.0f});
    fields.push_back({"Ultimate object", InspectorField::FieldType::InputText, &ultimateName});
    fields.push_back({"Ultimate damage", InspectorField::FieldType::Int, &ultimateDamage, 0.0f, 5.0f});
    fields.push_back({"Ultimate cooldown", InspectorField::FieldType::Float, &ultimateCd, 0.0f, 5.0f});
    fields.push_back({"Ultimate Animation delay", InspectorField::FieldType::Float, &ultimateAnimationDelay, 0.0f, 5.0f}
    );
    fields.push_back({"Ultimate hitbox delay", InspectorField::FieldType::Float, &ultimateHitboxDelay, 0.0f, 5.0f});
    fields.push_back({"Ultimate hitbox duration", InspectorField::FieldType::Float, &ultimateHitboxDuration, 0.0f, 5.0f}
    );
    fields.push_back({"Charged Attack object", InspectorField::FieldType::InputText, &chargedAttackName});
    fields.push_back({"Attack charging duration", InspectorField::FieldType::Float, &chargeDuration, 0.0f, 10.0f});
    fields.push_back({"Charged Attack damage", InspectorField::FieldType::Int, &chargedAttackDamage, 0.0f, 5.0f});
    fields.push_back(
        {"Charged Attack hitbox delay", InspectorField::FieldType::Float, &chargedAttackHitboxDelay, 0.0f, 5.0f}
    );
    fields.push_back(
        {"Charged Attack hitbox duration", InspectorField::FieldType::Float, &chargedAttackHitboxDuration, 0.0f, 5.0f}
    );
    fields.push_back({"Aim shadow object", InspectorField::FieldType::InputText, &aimShadowName});
    fields.push_back({"Melee trail object", InspectorField::FieldType::InputText, &meleeTrailName});
    fields.push_back({"Melee VFX object", InspectorField::FieldType::InputText, &meleeVfxName});
    fields.push_back({"Take mushroom cooldown", InspectorField::FieldType::Float, &takeMushroomCd, 0.0f, 5.0f});
    fields.push_back({"Mushroom healing", InspectorField::FieldType::Int, &mushroomHeal, 0.0f, 5.0f});
    fields.push_back({"Dash Trail object", InspectorField::FieldType::InputText, &dashTrailName});
    fields.push_back({"Dash decal object", InspectorField::FieldType::InputText, &dashDecalName});
    fields.push_back({"Dash decal disappear", InspectorField::FieldType::Float, &dashDecalTimer, 0.0f, 20.0f});
    fields.push_back({"God Mode", InspectorField::FieldType::Bool, &godMode});
}

bool CuChulainn::Init()
{
    // GLOG("Initiating CuChulainn");

    Character::Init();

    GameObject* healthUIObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("Health");
    if (healthUIObject)
    {
        healthImageComponent = healthUIObject->GetComponent<ImageComponent*>();
    }
    healthBarTextures = {
        1257129746400865, // 0HP
        1211032143220573, // 1HP
        1229536411852494, // 2HP
        1222839804934023, // 3HP
        1244849110337061, // 4HP
        1274246616335466, // 5HP
        1207603259151767, // 6HP
        1232318091978476, // 7HP
        1247873624040725, // 8HP
        1211992175790243, // 9HP
        1245070082308559  // 10HP

    };

    GameObject* dashUIObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("DashCooldown");
    if (dashUIObject) dashImageComponent = dashUIObject->GetComponent<ImageComponent*>();

    GameObject* ultimanteUIObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("UltimateCooldown");
    if (ultimanteUIObject) ultimateImageComponent = ultimanteUIObject->GetComponent<ImageComponent*>();

    playerScript = this;

    character    = parent->GetComponent<CharacterControllerComponent*>();
    if (!character) GLOG("CharacterController component not found for CuChulainn")
    else speed = character->GetSpeed();

    cameraObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(cameraName);
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

    const GameObject* spearObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(spearName);
    if (spearObj && spearObj->GetComponent<ScriptComponent*>())
    {
        spear = spearObj->GetComponent<ScriptComponent*>()->GetScriptByType<Projectile>();
        if (!spear) GLOG("[WARNING] No projectile found by the name %s", spearName.c_str());
    }

    chargedAttackCollider = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(chargedAttackName);
    if (!chargedAttackCollider) GLOG("[WARNING] No ultimate found for CuChualin")
    else chargedAttackCollider->SetEnabled(false);

    ultimateObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(ultimateName);
    if (!ultimateObject) GLOG("[WARNING] No ultimate found for CuChulain")
    else ultimateObject->SetEnabled(false);

    aimShadowObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(aimShadowName);
    if (!aimShadowObject) GLOG("[WARNING] No shadow found for aiming in CuChulain")
    else aimShadowObject->SetEnabled(false);

    meleeTrailObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(meleeTrailName);
    if (!meleeTrailObject) GLOG("[WARNING] No melee trail found for melee attack in CuChulain")
    else meleeTrailObject->SetEnabled(false);

    meleeVfxObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(meleeVfxName);
    if (!meleeVfxObject) GLOG("[WARNING] No melee VFX found for melee attack in CuChulain")
    else meleeVfxObject->SetEnabled(false);

    dashTrail = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(dashTrailName);
    if (!dashTrail) GLOG("[WARNING] No dash trail found for CuChulain")
    else dashTrail->SetEnabled(false);

    dashDecal = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(dashDecalName);
    if (!dashDecal) GLOG("[WARNING] No dash decal found for CuChulain")
    else dashDecal->SetEnabled(false);

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] CuChulainn: No audio component found");

    state = CharacterStates::IDLE;

    return true;
}

void CuChulainn::Update(float deltaTime)
{
    if (state == CharacterStates::DEATH)
    {
        deathTimer += deltaTime;
        if (deathTimer > 4.0f) Respawn();
    }

    if (isDead || !character) return;

    if (character->GetInputDown()) GetInputs();
    Character::Update(deltaTime);
    PerformAttack();
    CheckIsFalling();

    if (AppEngine->GetDebugDrawModule()->GetDebugOptionValue((int)DebugOptions::RENDER_DEBUG_VISUALS))
    {
        const std::string life           = "Health: " + std::to_string(currentHealth);
        const std::string animState      = "Anim state: " + stateName.GetString();
        const std::string logicState     = "Logic state: " + GetLogicStateName();
        const std::string mushroomsState = "Mushrooms: " + std::to_string(mushrooms);

        std::vector<std::pair<std::string, float2>> logs {
            {life,           float2(-50.0f, -140.0f)},
            {animState,      float2(-80.0f, -160.0f)},
            {logicState,     float2(-80.0f, -180.0f)},
            {mushroomsState, float2(-60.0f, -200.0f)},
        };

        RenderDebug(logs, float3(0.0f, 1.0f, 0.0f));
    }
}

void CuChulainn::OnDeath()
{
    // TODO: include death sound for the character
    if (healthImageComponent) healthImageComponent->ChangeTexture(healthBarTextures[0]);
    isAttacking = false;
    deathTimer  = 0.0f;
    if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
    if (state == CharacterStates::AIM && camera) camera->EnableAimOffset(false);
    character->EnableMovement(false);
    state = CharacterStates::DEATH;
    if (animComponent) animComponent->UseTrigger("Death");
}

void CuChulainn::OnDamageTaken(int amount)
{
    UpdateHealthBarUI();
    if (camera) camera->StartShake(0.2f, 0.2f);

    if (state == CharacterStates::CHARGING)
    {
        character->EnableMovement(true);
        state = CharacterStates::IDLE;
        if (animComponent) animComponent->UseTrigger("Idle");
    }
    // TODO: play CuChulainn take damage sound
    // TODO: fill riastrad bar dinamically
}

void CuChulainn::OnHealed(int amount)
{
    UpdateHealthBarUI();
    // TODO: play CuChulainn recover sound
    // TODO: play particle system effects
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

    if (!isDashing && dashTrail) dashTrail->SetEnabled(false);

    UpdateDashCooldownUI();
    UpdateUltimateCooldownUI();

    if (desiredDash && CanDash()) Dash();
    else if (desiredHeal && CanHeal()) UseMushroom();
    else if (desiredUltimate && CanUltimate()) UltimateAttack();
    else if (desiredAttack && CanAttack()) Attack(deltaTime);
    else if (desiredAim && CanAim()) Aim(deltaTime);
    else if (attackPressTimer >= 0.2f && CanChargeAttack()) ChargeAttack();
    else if (state != CharacterStates::BASIC_ATTACK && !character->IsDashing() && state != CharacterStates::RESPAWN &&
             state != CharacterStates::AIM && state != CharacterStates::FALL && state != CharacterStates::ULTIMATE &&
             state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::CHARGING &&
             state != CharacterStates::HEAL)
        Move();

    // When finished animation, go back to idle state
    if (animComponent && animComponent->IsFinished())
    {
        if (stateName == HashString("Attack_1") || stateName == HashString("Attack_2") ||
            stateName == HashString("Attack_3") || stateName == HashString("Attack_4"))
        {
            if (isAttacking) comboBufferTimer = 0.1f;
            isAttacking = false;
            if(meleeVfxObject)meleeVfxObject->SetEnabled(false);
        }
        else if (stateName == HashString("Charge"))
        {
            animComponent->UseTrigger("Charge");
        }
        else
        {
            if (state == CharacterStates::ULTIMATE && ultimateObject->GetComponent<AnimationComponent*>()->IsPlaying())
                return;
            if (state == CharacterStates::CHARGED_ATTACK && meleeTrailObject) meleeTrailObject->SetEnabled(false);
            state = CharacterStates::IDLE;
            animComponent->UseTrigger("Idle");
        }
    }
}

void CuChulainn::GetInputs()
{
    if (AppEngine->GetGameTimer()->GetDeltaTime() <= 0.0f) return;

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

    if (direction.Length() < 0.55f) character->SetIsRunning(false);
    else character->SetIsRunning(true);
    direction = camFront * direction.z + camRight * direction.x;
    character->SetDirection(direction);

    // Heal
    if (keyboard[SDL_SCANCODE_E] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] == KEY_DOWN)
    {
        desiredTakeMushroom = true;
        takeMushroomCdTimer = takeMushroomCd;
    }
    if (keyboard[SDL_SCANCODE_R] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] == KEY_DOWN)
    {
        desiredHeal = true;
        healCdTimer = healCooldown;
    }

    // Dash
    if (keyboard[SDL_SCANCODE_SPACE] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        desiredDash     = true;
        dashBufferTimer = inputBuffer;
    }

    // Attack
    if (mouse[SDL_BUTTON_LEFT - 1] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_X] == KEY_DOWN)
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
}

bool CuChulainn::CanDash() const
{
    bool canDash = dashTimer <= 0 && state != CharacterStates::AIM && !isAttacking && state != CharacterStates::FALL &&
                   state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE &&
                   state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::TAKE_MUSHROOM &&
                   state != CharacterStates::HEAL;

    if (canDash && state == CharacterStates::BASIC_ATTACK) canDash = comboBufferTimer > 0.0f;

    return canDash;
}

bool CuChulainn::CanAttack() const
{
    return state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
           state != CharacterStates::RESPAWN && comboCounter <= 1 && attackCdTimer <= 0.0f &&
           state != CharacterStates::ULTIMATE && state != CharacterStates::CHARGED_ATTACK &&
           state != CharacterStates::CHARGING && state != CharacterStates::TAKE_MUSHROOM &&
           state != CharacterStates::HEAL;
}

bool CuChulainn::CanUltimate() const
{
    bool canUltimate = state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
                       state != CharacterStates::RESPAWN && ultimateCdTimer <= 0.0f &&
                       state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::TAKE_MUSHROOM &&
                       state != CharacterStates::HEAL;

    if (canUltimate && state == CharacterStates::BASIC_ATTACK) canUltimate = comboBufferTimer > 0.0f;

    return canUltimate;
}

bool CuChulainn::CanTakeMushroom() const
{
    return state != CharacterStates::DASH && !isAttacking && state != CharacterStates::AIM &&
           state != CharacterStates::RESPAWN && state != CharacterStates::DEATH && state != CharacterStates::FALL &&
           state != CharacterStates::ULTIMATE && state != CharacterStates::HEAL;
}

bool CuChulainn::CanHeal() const
{
    return state != CharacterStates::DASH && !isAttacking && state != CharacterStates::AIM &&
           state != CharacterStates::RESPAWN && state != CharacterStates::DEATH && state != CharacterStates::FALL &&
           state != CharacterStates::ULTIMATE && state != CharacterStates::TAKE_MUSHROOM && mushrooms > 0;
}

bool CuChulainn::CanAim() const
{
    return state != CharacterStates::DASH && state != CharacterStates::BASIC_ATTACK && throwTimer <= 0 &&
           state != CharacterStates::FALL && state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE &&
           state != CharacterStates::CHARGED_ATTACK && state != CharacterStates::CHARGING &&
           state != CharacterStates::TAKE_MUSHROOM && state != CharacterStates::HEAL;
}

bool CuChulainn::CanChargeAttack() const
{
    bool canChargeAttack = state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
                           state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE &&
                           state != CharacterStates::AIM && state != CharacterStates::CHARGED_ATTACK &&
                           state != CharacterStates::TAKE_MUSHROOM && state != CharacterStates::HEAL;

    if (canChargeAttack && state == CharacterStates::BASIC_ATTACK) canChargeAttack = comboBufferTimer > 0.0f;

    return canChargeAttack;
}

void CuChulainn::UpdateTimers(float deltaTime)
{
    weaponCollider->SetEnabled(false);
    Character::UpdateTimers(deltaTime);

    // Dash timers
    dashTimer -= deltaTime;
    if (dashTimer < 0.0f) dashTimer = 0.0f;
    if (desiredDash)
    {
        dashBufferTimer -= deltaTime;
        if (dashBufferTimer < 0.0f) desiredDash = false;
    }

    // Dash decal
    dashDecalBufferTimer -= deltaTime;
    if (dashDecalBufferTimer < 0.0f)
    {
        if (dashDecal) dashDecal->SetEnabled(false);

        dashDecalBufferTimer = 0.0f;
    }

    // Melee attack timers
    if (desiredAttack)
    {
        attackBufferTimer -= deltaTime;
        if (attackBufferTimer < 0.0f) desiredAttack = false;
    }

    // Ranged attack timers
    desiredAim  = false;
    throwTimer -= deltaTime;
    if (throwTimer < 0.0f)
    {
        if (resetWeapon)
        {
            weapon->SetEnabled(true);
            resetWeapon = false;
        }
        throwTimer = 0.0f;
    }

    // Take mushrooms timers
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

    ultimateCdTimer -= deltaTime;
    if (ultimateCdTimer <= 0.0f) ultimateCdTimer = 0.0f;
    if (desiredUltimate)
    {
        ultimateBufferTimer -= deltaTime;
        if (ultimateBufferTimer < 0.0f) desiredUltimate = false;
    }

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

    if (state == CharacterStates::ULTIMATE) ultimateTimer += deltaTime;
    if (state == CharacterStates::CHARGED_ATTACK) chargedAttackTimer += deltaTime;
    if (state == CharacterStates::IDLE) idleTimer += deltaTime;

    if (state == CharacterStates::DASH) isInvulnerable = true;

    isDashing = state == CharacterStates::DASH ? true : false;
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
    if (verticalSpeed <= -3.0f && !character->IsGrounded() && animComponent)
    {
        animComponent->UseTrigger("Fall");
        state = CharacterStates::FALL;
    }

    if (state == CharacterStates::FALL && verticalSpeed >= -1.0f)
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
    if (audio) audio->EmitEvent(AK::EVENTS::ICE_BLAST);
    animComponent->OnResume();
    aimTimer   = 0.0f;

    throwTimer = throwCooldown;
    if (weapon)
    {
        weapon->SetEnabled(false);
        resetWeapon = true;
    }
    if (aimShadowObject) aimShadowObject->SetEnabled(false);

    spear->Shoot(parent->GetPosition(), character->GetFrontDirection());
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
    desiredDash      = false;
    state            = CharacterStates::DASH;

    // GLOG("DASH");

    dashTimer        = dashCooldown;
    lastDashStartPos = parent->GetGlobalTransform().TranslatePart();
    LookAtLeftStick();
    character->StartDash();
    isDashing = true;
    if (animComponent) animComponent->UseTrigger("Dash");
    if (dashTrail) dashTrail->SetEnabled(true);
    if (dashDecal)
    {
        dashDecal->SetEnabled(true);
        const float3 scale  = dashDecal->GetLocalTransform().ExtractScale();
        const Quat rotation = Quat::LookAt(float3::unitY, character->GetFrontDirection(), float3::unitZ, float3::unitY);
        const float3 pos    = lastDashStartPos + 2.5f * character->GetFrontDirection().Normalized();
        const float4x4 decalTransform = float4x4::FromTRS(pos, rotation, scale);
        dashDecal->SetLocalTransform(decalTransform);
        dashDecalBufferTimer = dashDecalTimer;
    }
}

void CuChulainn::PerformAttack()
{
    if (isAttacking && state == CharacterStates::BASIC_ATTACK)
    {
        if (attackTimer < attackHitboxDelay)
        {
            float distance = comboCounter == 2 ? 10.0f : 5.0f;
            character->MoveTo(distance);
        }
        else if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
                 attackTimer < attackHitboxDelay + attackHitboxDuration)
        {
            weaponCollider->SetEnabled(true);
        }
        else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
        {
            weaponCollider->SetEnabled(false);
        }
    }
    else if (state == CharacterStates::ULTIMATE)
    {
        if (!ultimateObject->IsEnabled() && ultimateTimer >= ultimateAnimationDelay)
        {
            ultimateObject->SetEnabled(true);
            ultimateObject->GetComponent<SphereColliderComponent*>()->SetEnabled(false);
            ultimateObject->GetComponent<AnimationComponent*>()->OnPlay(false);
        }
        else if (ultimateObject->IsEnabled() && ultimateTimer >= ultimateHitboxDelay + ultimateAnimationDelay &&
                 ultimateTimer < ultimateHitboxDelay + ultimateHitboxDuration + ultimateAnimationDelay)
        {
            ultimateObject->GetComponent<SphereColliderComponent*>()->SetEnabled(true);
        }
        else if (ultimateObject->IsEnabled() &&
                 ultimateTimer >= ultimateHitboxDelay + ultimateHitboxDuration + ultimateAnimationDelay)
        {
            ultimateObject->SetEnabled(false);
            ultimateObject->GetComponent<SphereColliderComponent*>()->SetEnabled(false);
            ultimateObject->GetComponent<AnimationComponent*>()->OnStop();
            ultimateTimer = 0.f;
            if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
        }
    }
    else if (state == CharacterStates::CHARGED_ATTACK)
    {
        if (!chargedAttackCollider->IsEnabled() && chargedAttackTimer >= chargedAttackHitboxDelay &&
            chargedAttackTimer < chargedAttackHitboxDelay + chargedAttackHitboxDuration)
        {
            chargedAttackCollider->SetEnabled(true);
        }
        else if (chargedAttackCollider->IsEnabled() &&
                 chargedAttackTimer >= chargedAttackHitboxDelay + chargedAttackHitboxDuration)
        {
            chargedAttackCollider->SetEnabled(false);
        }
    }
}

void CuChulainn::Attack(float deltaTime)
{
    // TODO: play basicAttack sound

    // GLOG("ATTACK");

    if (state == CharacterStates::AIM && camera)
    {
        if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
        camera->EnableAimOffset(false);
    }

    desiredAttack = false;
    state         = CharacterStates::BASIC_ATTACK;
    character->EnableMovement(false);
    if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
    if (meleeVfxObject) meleeVfxObject->SetEnabled(true);
    ++comboCounter;
    // GLOG("Combo counter: %d", comboCounter);

    Character::Attack(deltaTime);
    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    else LookAtLeftStick();
    if (animComponent)
    {
        const std::string trigger = "Attack" + std::to_string(comboCounter);
        animComponent->UseTrigger(trigger);
    }
}

void CuChulainn::UltimateAttack()
{
    // GLOG("ULTIMATEEEE");
    if (state == CharacterStates::AIM && camera)
    {
        camera->EnableAimOffset(false);
        if (meleeTrailObject) meleeTrailObject->SetEnabled(false);
    }
    state = CharacterStates::ULTIMATE;
    character->EnableMovement(false);
    if (meleeTrailObject) meleeTrailObject->SetEnabled(true);
    ultimateTimer   = 0.0f;
    ultimateCdTimer = ultimateCd;
    desiredUltimate = false;

    if (animComponent) animComponent->UseTrigger("Ultimate");
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
    desiredAim  = false;

    aimTimer   += deltaTime;
    if (aimTimer >= 0.07f) animComponent->OnPause();

    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    else LookAtLeftStick();
}

void CuChulainn::Move()
{
    character->EnableMovement(true);
    if (character->GetSpeed() > 0.5f)
    {
        if (state != CharacterStates::RUN && animComponent) animComponent->UseTrigger("Walk");
        state = CharacterStates::RUN;
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
    // TODO: This function will be called by the UI in the future

    Character::Restart();

    GameObject* healthUIObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("HealthBar");
    if (healthUIObject)
    {
        healthImageComponent = healthUIObject->GetComponent<ImageComponent*>();
        healthImageComponent->ChangeTexture(healthBarTextures[9]);
    }

    isDead        = false;
    currentHealth = reservedHealth;
    state         = CharacterStates::RESPAWN;
    SetPosition(spawnPos);
    if (animComponent) animComponent->UseTrigger("Respawn");
    character->EnableMovement(false);
}

void CuChulainn::TakeDamage(int amount)
{
    if (godMode) return;
    Character::TakeDamage(amount);
}

bool CuChulainn::TakeMushroom()
{
    bool taken = false;
    if (mushrooms <= 2)
    {
        mushrooms += 1;
        state      = CharacterStates::TAKE_MUSHROOM;
        taken      = true;
        // TODO: take mushrooms anim (maybe not animation), vfx etc
    }

    desiredTakeMushroom = false;

    return taken;
}

void CuChulainn::UseMushroom()
{
    mushrooms   -= 1;

    state        = CharacterStates::HEAL;
    desiredHeal  = false;

    // if (animComponent) animComponent->UseTrigger("Heal");
    if (animComponent) animComponent->UseTrigger("Ultimate");
    character->EnableMovement(false);

    Heal(mushroomHeal);

    // UpdateMushroomsUI();
}

void CuChulainn::UpdateHealthBarUI()
{
    if (!healthImageComponent || healthBarTextures.empty()) return;
    healthImageComponent->ChangeTexture(healthBarTextures[currentHealth]);
}

void CuChulainn::UpdateDashCooldownUI()
{
    const UID readyTex    = 1258786293084191;
    const UID cooldownTex = 1288043360624471;

    if (dashImageComponent) dashImageComponent->ChangeTexture(dashTimer > 0.0f ? cooldownTex : readyTex);
}

void CuChulainn::UpdateUltimateCooldownUI()
{
    const UID readyTex    = 1203132322652717;
    const UID cooldownTex = 1297453458525874;

    if (ultimateImageComponent) ultimateImageComponent->ChangeTexture(ultimateCdTimer > 0.0f ? cooldownTex : readyTex);
}
void CuChulainn::ChargeAttack()
{
    if (state != CharacterStates::CHARGING)
    {
        // GLOG("START CHARGING ATTACK");
        state       = CharacterStates::CHARGING;
        chargeTimer = chargeDuration;
        character->EnableMovement(false);

        if (animComponent) animComponent->UseTrigger("Charge");
    }
    else if (desiredChargedAttack)
    {
        desiredChargedAttack = false;
        isChargingAttack     = false;

        // GLOG("DESIRED CHARGE ATTACK");

        if (chargeTimer <= 0.0f)
        {
            GLOG("CHARGED ATTACK")

            state              = CharacterStates::CHARGED_ATTACK;
            chargedAttackTimer = 0.0f;
            if (meleeTrailObject) meleeTrailObject->SetEnabled(true);

            if (animComponent) animComponent->UseTrigger("Attack");
        }
        else
        {
            GLOG("NOT CHARGED ENOUFGH");
            character->EnableMovement(true);
            state = CharacterStates::IDLE;
            if (animComponent) animComponent->UseTrigger("Idle");
        }
    }
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
    default:
        return "MISSING!";
        break;
    }
}
