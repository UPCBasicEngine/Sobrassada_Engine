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
    if (!fireball) GLOG("[WARNING] No fireball found by the name: %s", fireballName.c_str())
    else fireball->SetEnabled(false);

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !trapMesh || !damageArea || !fireball) return;

    float gameTime = AppEngine->GetGameTimer()->GetTime() / 1000.0f;

    float distance = character->GetLastPosition().Distance(parent->GetPosition());
    if (distance <= activationRange)
    {
        activated = true;
        trapMesh->SetEnabled(true);
    }
    else activated = false;

    if (activated)
    {
        if (!attacking && gameTime - lastAttackTime >= attackCooldown)
        {
            fireball->SetLocalPosition(parent->GetPosition() + float3(0.0f, 20.f, 0.0f));
            fireball->SetEnabled(true);
            lastAttackTime = gameTime;
            verticalSpeed  = 0.0f;
            attacking      = true;
        }
    }
    else if (!attacking && !damageActive) trapMesh->SetEnabled(false);

    if (attacking)
    {
        float4x4 newTransform = fireball->GetLocalTransform();
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
        fireball->UpdateTransformForGOBranch();

        if (currentPos.y <= parent->GetPosition().y) // impact
        {
            damageArea->SetEnabled(true);
            fireball->SetEnabled(false);
            attacking    = false;
            damageActive = true;
            lastHitTime  = gameTime;
        }
    }

    if (damageActive && gameTime - lastHitTime >= damageDuration) // disable damage
    {
        damageArea->SetEnabled(false);
        damageActive = false;
    }
}