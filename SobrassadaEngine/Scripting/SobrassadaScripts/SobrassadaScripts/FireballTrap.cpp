#include "pch.h"
#undef max
#undef min

#include "Application.h"
#include "CuChulainn.h"
#include "FireballTrap.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Standalone/CharacterControllerComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

#include <algorithm>

FireballTrap::FireballTrap(GameObject* parent) : Script(parent)
{
    fields.push_back({"Trap Activated", InspectorField::FieldType::Bool, &activated});
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &activationRange, 0.0f, 20.0f});
    fields.push_back({"Attack Cooldown", InspectorField::FieldType::Float, &attackCooldown, 0.0f, 10.0f});
    fields.push_back({"Trap Damage", InspectorField::FieldType::Int, &damage, 0, 5});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 4.0f});
    fields.push_back({"Fireball Name", InspectorField::FieldType::InputText, &fireballName});
    fields.push_back({"Rotation Speed", InspectorField::FieldType::Float, &rotationSpeed, 0.0f, 4.0f});
    fields.push_back({"Falling Height", InspectorField::FieldType::Float, &fallingHeight, 0.0f, 50.0f});
    fields.push_back({"Max Fall Speed", InspectorField::FieldType::Float, &editableMaxFallSpeed, 0.0f, 40.0f});
}

bool FireballTrap::Init()
{
    groundMesh = parent->GetComponent<MeshComponent*>();
    if (groundMesh) groundMesh->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without mesh component.");

    damageCollider = parent->GetComponent<SphereColliderComponent*>();
    if (damageCollider) damageCollider->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without sphere collider component.");

    fireball = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(fireballName);
    if (fireball) fireball->SetEnabled(false);
    else GLOG("[WARNING] No fireball found by the name: %s", fireballName.c_str())

    lastAttackTime += -attackCooldown;

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !groundMesh || !damageCollider || !fireball) return;

    const float gameTime = AppEngine->GetGameTimer()->GetTime() / 1000.0f;
    maxFallSpeed         = -editableMaxFallSpeed;

    const float distance = character->GetLastPosition().Distance(parent->GetPosition());
    activated            = (distance <= activationRange);

    if (!activated && !attacking && !isDealingDamage) return;

    if (activated && !attacking && gameTime - lastAttackTime >= attackCooldown)
    {
        StartAttack(gameTime);
    }

    if (!hasImpacted)
    {
        UpdateFireball(deltaTime);
    }

    if (attacking && fireball->GetGlobalTransform().TranslatePart().y <= parent->GetGlobalTransform().TranslatePart().y)
    {
        HandleImpact(gameTime);
    }

    if (isDealingDamage && gameTime - lastHitTime >= damageDuration)
    {
        DisableDamage();
    }
}

void FireballTrap::StartAttack(float gameTime)
{
    fireball->SetEnabled(true);
    const float3 startPos = parent->GetPosition() + float3(0.0f, fallingHeight, 0.0f);
    fireball->SetLocalPosition(startPos);

    lastAttackTime = gameTime;
    verticalSpeed  = 0.0f;
    attacking      = true;
    hasImpacted    = false;
}

void FireballTrap::HandleImpact(float gameTime)
{

    groundMesh->SetEnabled(true);
    damageCollider->SetEnabled(true);

    attacking             = false;
    isDealingDamage       = true;
    lastHitTime           = gameTime;
    hasImpacted           = true;

    const float3 startPos = parent->GetPosition() + float3(0.0f, 100.0f, 0.0f);
    fireball->SetLocalPosition(startPos);
}

void FireballTrap::DisableDamage()
{
    groundMesh->SetEnabled(false);
    damageCollider->SetEnabled(false);

    isDealingDamage = false;
}

void FireballTrap::UpdateFireball(float deltaTime)
{
    float4x4 newTransform = fireball->GetGlobalTransform();
    float3 currentPos     = newTransform.TranslatePart();

    if (deltaTime < 0.1f)
    {
        verticalSpeed += gravity * deltaTime;
        verticalSpeed  = std::max(verticalSpeed, maxFallSpeed); // Clamp fall speed

        currentPos.y  += (verticalSpeed * deltaTime);
    }

    newTransform = newTransform * float4x4::RotateX(rotationSpeed * deltaTime);
    newTransform.SetTranslatePart(currentPos);

    fireball->SetLocalTransform(newTransform);
}
