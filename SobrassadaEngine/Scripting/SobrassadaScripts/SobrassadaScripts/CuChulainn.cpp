#include "pch.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraMovement.h"
#include "Component.h"
#include "CuChulainn.h"
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

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] CuChulainn: No audio component found");

    return true;
}

void CuChulainn::Update(float deltaTime)
{
    // TODO: Maybe instead of this call it at the end of death animation (the current animations lasts forever)
    if (state == CharacterStates::DEATH)
    {
        deathTimer += deltaTime;
        if (deathTimer > 5.0f) parent->SetEnabled(false);
    }

    if (isDead || !character) return;

    if (character->GetInputDown()) GetInputs();
    Character::Update(deltaTime);
    PerformAttack();
    CheckIsFalling();
}

void CuChulainn::OnDeath()
{
    // TODO: include death sound for the character

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
    else if (desiredAttack && CanAttack()) Attack(deltaTime);
    else if (desiredAim && CanAim()) Aim(deltaTime);
    else if (!isAttacking && !character->IsDashing() && state != CharacterStates::RESPAWN &&
             state != CharacterStates::AIM && state != CharacterStates::FALL)
        Move();

    // When finished animation, go back to idle state
    if (animComponent && animComponent->IsFinished())
    {
        const HashString& stateName = animComponent->GetCurrentStateName();
        GLOG("Animation name: %s", stateName.GetString().c_str());

        if (stateName == HashString("Respawn") || stateName == HashString("Land"))
        {
            character->EnableMovement(true);
        }

        state = CharacterStates::IDLE;
        animComponent->UseTrigger("Idle");
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

    if (keyboard[SDL_SCANCODE_SPACE] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        desiredDash     = true;
        dashBufferTimer = dashBuffer;
    }
    if (mouse[SDL_BUTTON_LEFT - 1] == KEY_DOWN || controller[SDL_CONTROLLER_BUTTON_X] == KEY_DOWN)
    {
        desiredAttack     = true;
        attackBufferTimer = attackBuffer;
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

bool CuChulainn::CanDash()
{
    // TODO: Add more condifions if there are (Maybe dashing doesn't cancel attack animations, etc.)
    return dashTimer <= 0 && state != CharacterStates::AIM && state != CharacterStates::BASIC_ATTACK &&
           state != CharacterStates::FALL;
}

bool CuChulainn::CanAttack()
{
    return (state != CharacterStates::DASH && !isAttacking && state != CharacterStates::FALL);
}

bool CuChulainn::CanAim() const
{
    return (state != CharacterStates::DASH && !isAttacking && throwTimer <= 0 && state != CharacterStates::FALL);
}

void CuChulainn::UpdateTimers(float deltaTime)
{
    Character::UpdateTimers(deltaTime);

    // Dash timers
    dashTimer -= deltaTime;
    if (dashTimer < 0) dashTimer = 0;
    if (desiredDash)
    {
        dashBufferTimer -= deltaTime;
        if (dashBufferTimer < 0) desiredDash = false;
    }

    // Melee attack timers

    if (desiredAttack)
    {
        attackBufferTimer -= deltaTime;
        if (attackBufferTimer < 0) desiredAttack = false;
    }

    // Ranged attack timers
    desiredAim  = false;
    throwTimer -= deltaTime;
    if (throwTimer < 0)
    {
        if (resetWeapon)
        {
            weapon->SetEnabled(true);
            resetWeapon = false;
        }
        throwTimer = 0;
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

void CuChulainn::LookAtJoystick()
{
    const float2& stick    = AppEngine->GetInputModule()->GetRightStick();
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
    desiredDash = false;
    state       = CharacterStates::DASH;

    GLOG("DASH");

    // TODO: Dash
    dashTimer        = dashCooldown;
    lastDashStartPos = parent->GetGlobalTransform().TranslatePart();
    character->StartDash();
    if (animComponent) animComponent->UseTrigger("Dash");
}

void CuChulainn::PerformAttack()
{
    // TODO: make interaction with hitboxes with the enemy ones
    // TODO: activate and disable the box collider located on one on the gameobjects bones

    if (!isAttacking) return;

    if (attackTimer >= attackDuration) isAttacking = false;

    // TODO: When timer matches animation, enable weapon collider. Disable it afterwards
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

    Character::Attack(deltaTime);
    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    if (animComponent) animComponent->UseTrigger("Attack");
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
    else LookAtJoystick();
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
    state = CharacterStates::RESPAWN;
    SetPosition(spawnPos);
    if (animComponent) animComponent->UseTrigger("Respawn");
    character->EnableMovement(false);
    // TODO: Reset hitboxes, timers, enable, etc. If scene is reloaded then probably not needed
}