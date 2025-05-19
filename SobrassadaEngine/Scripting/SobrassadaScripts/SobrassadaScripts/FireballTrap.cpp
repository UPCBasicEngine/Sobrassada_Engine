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
    trapMesh = parent->GetComponent<MeshComponent*>();
    if (trapMesh) trapMesh->SetEnabled(false);
    else GLOG("[WARNING] FireTrap without mesh component.");

    damageArea = parent->GetComponent<SphereColliderComponent*>();
    if (damageArea) damageArea->SetEnabled(false);
    else GLOG("[WARNING] FireTrap without cube collider component.");

    fireball = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(fireballName);
    if (fireball) fireball->SetEnabled(false);
    else GLOG("[WARNING] No fireball found by the name: %s", fireballName.c_str())

    lastAttackTime += -attackCooldown;

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !trapMesh || !damageArea || !fireball) return;

    const float gameTime = AppEngine->GetGameTimer()->GetTime() / 1000.0f;
    maxFallSpeed         = -editableMaxFallSpeed;

    const float distance = character->GetLastPosition().Distance(parent->GetPosition());
    activated            = (distance <= activationRange);

    if (!activated && !attacking && !damageActive) return;

    float3 actualPos = fireball->GetGlobalTransform().TranslatePart();
    GLOG("Actual fireball position: %.2f", actualPos.y);

    // attack again
    if (activated && !attacking && !damageActive && gameTime - lastAttackTime >= attackCooldown)
    {
        fireball->SetEnabled(true);
        const float3 startPos = parent->GetPosition() + float3(0.0f, fallingHeight, 0.0f);
        fireball->SetLocalPosition(startPos);

        lastAttackTime = gameTime;
        verticalSpeed  = 0.0f;
        attacking      = true;
        impacted       = false;
    }

    // impact
    else if (attacking && fireball->GetPosition().y <= parent->GetPosition().y)
    {
        trapMesh->SetEnabled(true);
        damageArea->SetEnabled(true);

        attacking             = false;
        damageActive          = true;
        lastHitTime           = gameTime;
        impacted              = true;

        const float3 startPos = parent->GetPosition() + float3(0.0f, 200.0f, 0.0f);
        fireball->SetLocalPosition(startPos);
    }

    // disable damage
    else if (damageActive && gameTime - lastHitTime >= damageDuration)
    {
        trapMesh->SetEnabled(false);
        damageArea->SetEnabled(false);

        damageActive = false;
    }

    // update fireball
    else if (!impacted)
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
}