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
#include <cstdlib>
#include <ctime>

FireballTrap::FireballTrap(GameObject* parent) : Script(parent)
{
    fields.push_back({"Trap Activated", InspectorField::FieldType::Bool, &activated});
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &activationRange, 0.0f, 100.0f});
    fields.push_back({"Min Attack Cooldown", InspectorField::FieldType::Float, &minAttackCooldown, 0.0f, 10.0f});
    fields.push_back({"Max Attack Cooldown", InspectorField::FieldType::Float, &maxAttackCooldown, 0.0f, 30.0f});
    fields.push_back({"Trap Damage", InspectorField::FieldType::Int, &damage, 0, 5});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Rotation Speed", InspectorField::FieldType::Float, &rotationSpeed, 0.0f, 100.0f});
    fields.push_back({"Falling Height", InspectorField::FieldType::Float, &fallingHeight, 0.0f, 200.0f});
    fields.push_back({"Max Fall Speed", InspectorField::FieldType::Float, &editableMaxFallSpeed, 0.0f, 100.0f});
    fields.push_back({"Gravity", InspectorField::FieldType::Float, &editableGravity, 0.0f, 20.0f});
}

bool FireballTrap::Init()
{
    groundMesh = parent->GetComponent<MeshComponent*>();
    if (groundMesh) groundMesh->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without mesh component.");

    damageCollider = parent->GetComponent<SphereColliderComponent*>();
    if (damageCollider) damageCollider->SetEnabled(false);
    else GLOG("[WARNING] FireballTrap without sphere collider component.");

    if (parent->GetChildren().size() > 0)
    {
        fireball = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
        if (fireball) fireball->SetEnabled(false);
        else GLOG("[WARNING] No fireball found as child of base")
    }

    if (parent->GetChildren().size() > 1)
    {
        fireballShadow = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
        if (fireballShadow) fireballShadow->SetEnabled(false);
        else GLOG("[WARNING] No fireball shadow found as child of base")
    }

    lastAttackTime += -maxAttackCooldown;
    srand(static_cast<unsigned>(time(0))); // random seed

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    if (!character || !groundMesh || !damageCollider || !fireball) return;

    const float gameTime = AppEngine->GetGameTimer()->GetTime() / 1000.0f;
    maxFallSpeed         = -editableMaxFallSpeed;
    gravity              = -editableGravity;

    const float distance = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());
    activated            = (distance <= activationRange * activationRange);

    if (!activated && !attacking && !isDealingDamage) return;

    if (randomAttackTime < 0.0f) randomAttackTime = GenerateRandomAttackTime(minAttackCooldown, maxAttackCooldown);

    if (activated && !attacking && gameTime - lastAttackTime >= randomAttackTime)
    {
        StartAttack(gameTime);
    }

    if (attacking && !hasImpacted)
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
    fireball->SetLocalPosition(float3(0.0f, fallingHeight, 0.0f));

    if (fireballShadow != nullptr)  fireballShadow->SetEnabled(true);

    lastAttackTime = gameTime;
    verticalSpeed  = 0.0f;
    attacking      = true;
    hasImpacted    = false;

    // GLOG("Random time: %.2f", randomAttackTime);

    randomAttackTime = -1.0f;
}

void FireballTrap::HandleImpact(float gameTime)
{
    fireball->SetEnabled(false);
    if (fireballShadow != nullptr)  fireballShadow->SetEnabled(false);
    groundMesh->SetEnabled(true);
    damageCollider->SetEnabled(true);

    attacking       = false;
    isDealingDamage = true;
    lastHitTime     = gameTime;
    hasImpacted     = true;
}

void FireballTrap::DisableDamage()
{
    groundMesh->SetEnabled(false);
    damageCollider->SetEnabled(false);

    isDealingDamage = false;
}

void FireballTrap::UpdateFireball(float deltaTime)
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
}

float FireballTrap::GenerateRandomAttackTime(float min, float max)
{
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
