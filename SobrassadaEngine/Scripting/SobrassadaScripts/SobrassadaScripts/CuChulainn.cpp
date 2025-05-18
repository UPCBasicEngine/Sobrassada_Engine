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
}

bool CuChulainn::Init()
{
    // GLOG("Initiating CuChulainn");

    Character::Init();

    character = parent->GetComponent<CharacterControllerComponent*>();
    if (!character) GLOG("CharacterController component not found for CuChulainn")
    else speed = character->GetSpeed();

    const GameObject* cameraObj = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(cameraName);
    if (cameraObj && cameraObj->GetComponent<ScriptComponent*>())
    {
        camera = cameraObj->GetComponent<ScriptComponent*>()->GetScriptByType<CameraMovement>();
        if (!camera) GLOG("[WARNING] No camera found by the name %s", cameraName.c_str());
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
    if (isDead || !character) return;

    GetInputs();
    Character::Update(deltaTime);
    PerformAttack();
}

void CuChulainn::OnDeath()
{
    // TODO: include death sound for the character
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
    if (desiredDash && CanDash()) Dash();
    else if (desiredAttack && CanAttack()) Attack(deltaTime);
    else if (desiredAim && CanAim()) Aim();
    else if (!isAttacking && !isDashing) Move();

    // When finished animation, go back to idle state
    if (animComponent && animComponent->IsFinished())
    {
        state = CharacterStates::IDLE;
        animComponent->UseTrigger("idle");
    }
}

void CuChulainn::GetInputs()
{
    const InputModule* input   = AppEngine->GetInputModule();
    const KeyState* keyboard   = input->GetKeyboard();
    const KeyState* mouse      = input->GetMouseButtons();
    const KeyState* controller = input->GetControllerButtons();

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
        Respawn();
    }
    if (keyboard[SDL_SCANCODE_F6])
    {
        spawnPos = parent->GetPosition();
    }
}

bool CuChulainn::CanDash()
{
    // TODO: Add more condifions if there are (Maybe dashing doesn't cancel attack animations, etc.)
    return dashTimer <= 0;
}

bool CuChulainn::CanAttack()
{
    return (state != CharacterStates::DASH && !isAttacking);
}

bool CuChulainn::CanAim() const
{
    return (state != CharacterStates::DASH && !isAttacking && throwTimer <= 0);
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
    const float2 stick = AppEngine->GetInputModule()->GetRightStick();
    float3 direction   = float3(stick.x, 0, stick.y);
    if (direction.LengthSq() > 0.001f) character->LookAt(direction);
}

void CuChulainn::ThrowSpear()
{
    if (camera) camera->EnableAimOffset(false);
    if (audio) audio->EmitEvent(AK::EVENTS::ICE_BLAST);
    // GLOG("THROW SPEAR");
    throwTimer = throwCooldown;
    if (weapon)
    {
        weapon->SetEnabled(false);
        resetWeapon = true;
    }

    const auto a = character->GetFrontDirection();
    spear->Shoot(parent->GetPosition(), character->GetFrontDirection());
}

void CuChulainn::Dash()
{
    if (state == CharacterStates::AIM && camera) camera->EnableAimOffset(false);
    desiredDash = false;
    state       = CharacterStates::DASH;

    // TODO: Dash
    // character->Dash(direction)
    if (animComponent) animComponent->UseTrigger("dash");
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

    Character::Attack(time);
    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    if (animComponent) animComponent->UseTrigger("attack");
}

void CuChulainn::Aim()
{
    if (!spear) return;

    if (state != CharacterStates::AIM)
    {
        if (camera) camera->EnableAimOffset(true);
        state = CharacterStates::AIM;
        character->EnableMovement(false);
    }
    desiredAim = false;

    if (AppEngine->GetInputModule()->IsUsingKeyboard()) LookAtMouse();
    else LookAtJoystick();
    if (animComponent) animComponent->UseTrigger("aim");
}

void CuChulainn::Move()
{
    character->EnableMovement(true);
    if (character->GetSpeed() > 0.5f)
    {
        if (state != CharacterStates::RUN && animComponent) animComponent->UseTrigger("run");
        state = CharacterStates::RUN;
    }
    else
    {
        if (state != CharacterStates::IDLE && animComponent) animComponent->UseTrigger("idle");
        state = CharacterStates::IDLE;
    }
}

void CuChulainn::Respawn()
{
    parent->SetLocalPosition(spawnPos);
    if (camera) camera->SetPosition(spawnPos);
}