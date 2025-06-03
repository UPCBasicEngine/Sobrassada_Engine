#include "pch.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraMovement.h"
#include "Component.h"
#include "CuChulainn.h"
#include "DebugDrawModule.h"
#include "GameObject.h"
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

#include "SDL.h"
#include "Wwise_IDs.h"

CharacterControllerComponent* character = nullptr;

CuChulainn::CuChulainn(GameObject* parent)
    : Character(parent, 5, 1, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, CharacterType::CuChulainn)
{
    currentHealth = 3; // mainChar starts low hp

    // TODO: Replace target names by gameObjects when overriding prefabs doesn't break the link
    fields.push_back({"Camera Object Name", InspectorField::FieldType::InputText, &cameraName});
    fields.push_back({"Spear Projectile Name", InspectorField::FieldType::InputText, &spearName});
    fields.push_back({"Range attack cooldown", InspectorField::FieldType::Float, &throwCooldown, 0.0f, 2.0f});
    fields.push_back({"Dash cooldown", InspectorField::FieldType::Float, &dashCooldown, 0.0f, 5.0f});
    fields.push_back({"Ultimate object", InspectorField::FieldType::InputText, &ultimateName, 0.0f, 5.0f});
    fields.push_back({"Ultimate damage", InspectorField::FieldType::Int, &ultimateDamage, 0.0f, 5.0f});
    fields.push_back({"Ultimate cooldown", InspectorField::FieldType::Float, &ultimateCd, 0.0f, 5.0f});
}

bool CuChulainn::Init()
{
    // GLOG("Initiating CuChulainn");

    Character::Init();

    character = parent->GetComponent<CharacterControllerComponent*>();
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

    ultimateObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(ultimateName);
    if (!ultimateObject) GLOG("[WARNING] No ultimate found for CuChualin");

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] CuChulainn: No audio component found");

    return true;
}

void CuChulainn::Update(float deltaTime)
{
    // TODO: Some debug about life and current state
    AppEngine->GetDebugDrawModule()->Draw3DText(btVector3(-25, 2, -40), "XD moment");

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
}

bool CuChulainn::IsDead()
{
    return isDead;
}

void CuChulainn::OnDeath()
{
    // TODO: include death sound for the character

    deathTimer = 0.0f;
    character->EnableMovement(false);
    state = CharacterStates::DEATH;
    if (animComponent) animComponent->UseTrigger("Death");
}

void CuChulainn::OnDamageTaken(int amount)
{
    // TODO: play CuChulainn take damage sound
    // TODO: fill riastrad bar dinamically
}

void CuChulainn::OnHealed(int amount)
{
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

    if (desiredDash && CanDash()) Dash();
    else if (desiredUltimate && CanUltimate()) UltimateAttack();
    else if (desiredAttack && CanAttack()) Attack(deltaTime);
    else if (desiredAim && CanAim()) Aim(deltaTime);
    else if (state != CharacterStates::BASIC_ATTACK && !character->IsDashing() && state != CharacterStates::RESPAWN &&
             state != CharacterStates::AIM && state != CharacterStates::FALL && state != CharacterStates::ULTIMATE)
        Move();

    // TODO: Some transition in the dash or idle state, to continue the combo after a dash

    // When finished animation, go back to idle state
    if (animComponent && animComponent->IsFinished())
    {
        if (stateName == HashString("Attack_1") || stateName == HashString("Attack_2") ||
            stateName == HashString("Attack_3") || stateName == HashString("Attack_4"))
        {
            if (isAttacking) comboBufferTimer = 0.2f;
            isAttacking = false;
        }
        else
        {
            if (stateName == HashString("Ultimate") && ultimateObject) ultimateObject->SetEnabled(false);

            state = CharacterStates::IDLE;
            animComponent->UseTrigger("Idle");
        }
    }
}

void CuChulainn::GetInputs()
{
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

    direction = camFront * direction.z + camRight * direction.x;
    character->SetDirection(direction);

    if (keyboard[SDL_SCANCODE_E] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_B] == KEY_DOWN)
    {
        desiredHeal = true;
        healCdTimer = healCooldown;
    }
    if (keyboard[SDL_SCANCODE_SPACE] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        desiredDash     = true;
        dashBufferTimer = inputBuffer;
    }
    if (mouse[SDL_BUTTON_LEFT - 1] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_X] == KEY_DOWN)
    {
        desiredAttack     = true;
        attackBufferTimer = inputBuffer;
    }
    if (mouse[SDL_BUTTON_RIGHT - 1] == KEY_REPEAT || input->GetLeftTrigger().first == KEY_REPEAT)
    {
        desiredAim = true;
    }
    if (input->GetLeftTrigger().first == KEY_UP)
    {
        if (state == CharacterStates::AIM) camera->EnableAimOffset(false);
    }
    if (mouse[SDL_BUTTON_RIGHT - 1] == KEY_UP || input->GetRightTrigger().first == KEY_DOWN)
    {
        if (state == CharacterStates::AIM) ThrowSpear();
    }
    if (keyboard[SDL_SCANCODE_F] || controller[SDL_CONTROLLER_BUTTON_Y] == KEY_DOWN)
    {
        desiredUltimate     = true;
        ultimateBufferTimer = inputBuffer;
    }
    if (keyboard[SDL_SCANCODE_F5])
    {
        // TODO: This should be SetSpawnPos, Respawn is here to test
        // SetPosition(spawnPos);
        Respawn();
    }
    if (keyboard[SDL_SCANCODE_F6])
    {
        spawnPos = parent->GetGlobalTransform().TranslatePart();
    }
}

bool CuChulainn::CanDash() const
{
    bool canDash = dashTimer <= 0 && state != CharacterStates::AIM && !isAttacking && state != CharacterStates::FALL &&
                   state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE;

    if (canDash && state == CharacterStates::BASIC_ATTACK) canDash = comboBufferTimer >= 0.0f;

    return canDash;
}

bool CuChulainn::CanAttack() const
{
    return state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
           state != CharacterStates::RESPAWN && comboCounter <= 1 && attackCdTimer <= 0.0f &&
           state != CharacterStates::ULTIMATE;
}

bool CuChulainn::CanUltimate() const
{
    bool canUltimate = state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL &&
                       state != CharacterStates::RESPAWN && ultimateTimer <= 0.0f;

    if (canUltimate && state == CharacterStates::BASIC_ATTACK) canUltimate = comboBufferTimer >= 0.0f;

    return canUltimate;
}

bool CuChulainn::CanAim() const
{
    return state != CharacterStates::DASH && state != CharacterStates::BASIC_ATTACK && throwTimer <= 0 &&
           state != CharacterStates::FALL && state != CharacterStates::RESPAWN && state != CharacterStates::ULTIMATE;
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

    if (!isAttacking && comboBufferTimer > 0.0f)
    {
        comboBufferTimer -= deltaTime;
        if (comboBufferTimer <= 0.0f)
        {
            comboCounter = -1;
            if (animComponent) animComponent->UseTrigger("AttackEnd");
            attackCdTimer = attackCooldown;
        }
    }

    ultimateTimer -= deltaTime;
    if (ultimateTimer <= 0.0f) ultimateTimer = 0.0f;
    if (desiredUltimate)
    {
        ultimateBufferTimer -= deltaTime;
        if (ultimateBufferTimer < 0.0f) desiredUltimate = false;
    }
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

void CuChulainn::LookAtRightstick()
{
    const float2& stick    = AppEngine->GetInputModule()->GetRightStick();
    const float3 direction = camFront * stick.y + camRight * stick.x;
    if (direction.LengthSq() > 0.001f) character->LookAt(direction);
}

void CuChulainn::LookAtLeftstick()
{
    const float2& stick    = AppEngine->GetInputModule()->GetLeftStick();
    const float3 direction = camFront * stick.y + camRight * stick.x;
    if (direction.LengthSq() > 0.001f) character->LookAt(direction);
}

void CuChulainn::CheckIsFalling()
{
    const float verticalSpeed = character->GetRealSpeed().y;

    // GLOG("Vertical speed %f", verticalSpeed);
    if (verticalSpeed <= -2.0f && !character->IsGrounded() && animComponent)
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
    if (audio) audio->EmitEvent(AK::EVENTS::ICE_BLAST);
    animComponent->OnResume();
    aimTimer   = 0.0f;

    throwTimer = throwCooldown;
    if (weapon)
    {
        weapon->SetEnabled(false);
        resetWeapon = true;
    }

    spear->Shoot(parent->GetPosition(), character->GetFrontDirection());
}

void CuChulainn::Dash()
{
    if (state == CharacterStates::AIM && camera) camera->EnableAimOffset(false);
    else if (state == CharacterStates::BASIC_ATTACK)
    {
        comboBufferTimer = character->GetDashDuration() + 0.1f;
        isAttacking      = false;
    }
    desiredDash = false;
    state       = CharacterStates::DASH;

    GLOG("DASH");

    dashTimer        = dashCooldown;
    lastDashStartPos = parent->GetGlobalTransform().TranslatePart();
    LookAtLeftstick();
    character->StartDash();
    if (animComponent) animComponent->UseTrigger("Dash");
}

void CuChulainn::PerformAttack()
{
    if (!isAttacking) return;

    // if (attackTimer >= attackDuration) isAttacking = false;

    if (!weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay &&
        attackTimer < attackHitboxDelay + attackHitboxDuration)
    {
        weaponCollider->SetEnabled(true);
    }
    else if (weaponCollider->GetEnabled() && attackTimer >= attackHitboxDelay + attackHitboxDuration)
    {
        weaponCollider->SetEnabled(false);
    }
}

void CuChulainn::Attack(float deltaTime)
{
    // TODO: play basicAttack sound

    // GLOG("ATTACK");

    if (state == CharacterStates::AIM && camera) camera->EnableAimOffset(false);
    desiredAttack = false;
    state         = CharacterStates::BASIC_ATTACK;
    character->EnableMovement(false);
    ++comboCounter;
    // GLOG("Combo counter: %d", comboCounter);

    Character::Attack(deltaTime);
    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    if (animComponent)
    {
        const std::string trigger = "Attack" + std::to_string(comboCounter);
        animComponent->UseTrigger(trigger);
    }
}

void CuChulainn::UltimateAttack()
{
    GLOG("ULTIMATEEEE");
    if (state == CharacterStates::AIM && camera) camera->EnableAimOffset(false);
    state = CharacterStates::ULTIMATE;
    character->EnableMovement(false);
    ultimateTimer   = ultimateCd;
    desiredUltimate = false;

    if (animComponent) animComponent->UseTrigger("Ultimate");

    // TODO: When the animation exists, trigger this according to it (like in PerformAttack())
    if (ultimateObject) ultimateObject->SetEnabled(true);
}

void CuChulainn::Aim(float deltaTime)
{
    if (!spear) return;

    if (state != CharacterStates::AIM)
    {
        if (camera) camera->EnableAimOffset(true);
        state = CharacterStates::AIM;
        character->EnableMovement(false);
        if (animComponent) animComponent->UseTrigger("Ranged");
    }
    desiredAim  = false;

    aimTimer   += deltaTime;
    if (aimTimer >= 0.1f) animComponent->OnPause();

    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    else LookAtRightstick();
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
        if (state != CharacterStates::IDLE && animComponent) animComponent->UseTrigger("Idle");
        state = CharacterStates::IDLE;
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

    isDead        = false;
    currentHealth = reservedHealth;
    state         = CharacterStates::RESPAWN;
    SetPosition(spawnPos);
    if (animComponent) animComponent->UseTrigger("Respawn");
    character->EnableMovement(false);
}