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

#include <Math/MathConstants.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>

FireballTrap::FireballTrap(GameObject* parent) : Script(parent)
{
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &cfg.activationRange, 0.0f, 100.0f});
    fields.push_back({"Min Attack Cooldown", InspectorField::FieldType::Float, &cfg.minAttackCooldown, 0.0f, 10.0f});
    fields.push_back({"Max Attack Cooldown", InspectorField::FieldType::Float, &cfg.maxAttackCooldown, 0.0f, 30.0f});
    fields.push_back({"Trap Damage", InspectorField::FieldType::Int, &damage, 0, 5});
    fields.push_back({"Damage Duration", InspectorField::FieldType::Float, &damageDuration, 0.0f, 10.0f});
    fields.push_back({"Rotation Speed", InspectorField::FieldType::Float, &rotationSpeed, 0.0f, 100.0f});
    fields.push_back({"Falling Height", InspectorField::FieldType::Float, &fallingHeight, 0.0f, 200.0f});
    fields.push_back({"Max Fall Speed", InspectorField::FieldType::Float, &editableMaxFallSpeed, 0.0f, 100.0f});
    fields.push_back({"Gravity", InspectorField::FieldType::Float, &editableGravity, 0.0f, 20.0f});
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &cfg.activationRange, 0.0f, 100.0f});
    fields.push_back({"Min Attack Cd", InspectorField::FieldType::Float, &cfg.minAttackCooldown, 0.0f, 10.0f});
    fields.push_back({"Mini Prototype", InspectorField::FieldType::GameObject, &miniPrototype, 0.f, 0.f});
    fields.push_back({"Mini Pool Size", InspectorField::FieldType::Int, &poolSize, 1.f, 50.f});
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

    srand(static_cast<unsigned>(time(0))); // random seed

    if (miniPrototype)
    {
        miniPrototype->SetEnabled(false);
        GLOG("Mini OK")
    }
    else
    {
        GLOG("[WARNING] FireballTrap: Mini prototype reference not set");
    }

    if (miniPrototype)
    {
        miniPrototype->SetEnabled(false);

        miniPool.reserve(poolSize);
        for (uint32_t i = 0; i < poolSize; ++i)
        {
            // same parent and local transform  as prototype
            GameObject* clone = new GameObject(parent->GetUID(), miniPrototype);
            clone->SetEnabled(false);
            parent->AddChildren(clone->GetUID());
            AppEngine->GetSceneModule()->GetScene()->AddGameObject(clone->GetUID(), clone);

            miniPool.push_back(clone);
        }
    }
    else
    {
        GLOG("[WARNING] FireballTrap: Mini prototype reference not set");
    }

    return true;
}

void FireballTrap::Update(float deltaTime)
{
    // TODO Make all movement depended on the deltaTime
    if (!character || !groundMesh || !damageCollider || !fireball) return;

    float distance;
    switch (activationState)
    {
    case SLEEPING:
        distance = character->GetLastPosition().DistanceSq(parent->GetGlobalTransform().TranslatePart());
        if (distance <= cfg.activationRange * cfg.activationRange)
        {
            randomAttackTime = GenerateRandomAttackTime(cfg.minAttackCooldown, cfg.maxAttackCooldown);
            activatedTime    = 0.0f;
            activationState  = IDLE;
        }
        break;
    case IDLE:
        activatedTime += deltaTime;
        if (activatedTime >= randomAttackTime) StartAttack();
        break;
    case DROPPING:
        UpdateFireball(deltaTime);
        break;
    case DAMAGING:
        activatedTime += deltaTime;
        if (activatedTime - randomAttackTime >= damageDuration) // Not fully accurate, but probably ok
            DisableDamage();
        break;
    }
}

void FireballTrap::StartAttack()
{
    fireball->SetEnabled(true);
    fireball->SetLocalPosition(float3(0.0f, fallingHeight, 0.0f));

    GameObject* mini = RequestMini();
    mini->SetLocalPosition(float3(0, fallingHeight, 0));

    if (fireballShadow != nullptr) fireballShadow->SetEnabled(true);

    verticalSpeed   = 0.0f;
    activationState = DROPPING;
}

void FireballTrap::HandleImpact()
{
    fireball->SetEnabled(false);
    if (fireballShadow != nullptr) fireballShadow->SetEnabled(false);
    groundMesh->SetEnabled(true);
    damageCollider->SetEnabled(true);

    activationState = DAMAGING;
}

void FireballTrap::DisableDamage()
{
    groundMesh->SetEnabled(false);
    damageCollider->SetEnabled(false);

    activationState = SLEEPING;
}

void FireballTrap::UpdateFireball(float deltaTime)
{
    float4x4 newTransform = fireball->GetLocalTransform();
    float3 currentPos     = newTransform.TranslatePart();

    if (deltaTime < 0.5f)
    {
        verticalSpeed = std::min(verticalSpeed + editableGravity * deltaTime, editableMaxFallSpeed); // Clamp fall speed
        currentPos.y -= verticalSpeed * deltaTime;
    }

    if (currentPos.y <= 0)
    {
        HandleImpact();
    }
    else
    {
        newTransform = newTransform * float4x4::RotateX(rotationSpeed * deltaTime);
        newTransform.SetTranslatePart(currentPos);

        fireball->SetLocalTransform(newTransform);
    }
}

float FireballTrap::GenerateRandomAttackTime(float min, float max)
{
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

GameObject* FireballTrap::RequestMini()
{
    for (GameObject* go : miniPool)
        if (!go->IsEnabled()) // free
        {
            go->SetEnabled(true);
            return go;
        }
    // pool exhausted? (optional) create a new one
    GameObject* clone = new GameObject(parent->GetUID(), miniPrototype);
    parent->AddChildren(clone->GetUID());
    AppEngine->GetSceneModule()->GetScene()->AddGameObject(clone->GetUID(), clone);

    clone->SetEnabled(true);
    miniPool.push_back(clone);
    return clone;
}

void FireballTrap::RecycleMini(GameObject* mini)
{
    if (!mini) return;
    mini->SetEnabled(false);
}
