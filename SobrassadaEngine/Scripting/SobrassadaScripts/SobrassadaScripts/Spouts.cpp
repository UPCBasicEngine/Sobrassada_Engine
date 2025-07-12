#include "pch.h"

#include "Character.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Spouts.h"
#include "Standalone/Physics/SphereColliderComponent.h"

Spouts::Spouts(GameObject* parent) : Script(parent)
{
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &activationRange, 0.0f, 100.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 5});
    fields.push_back({"Charging Duration", InspectorField::FieldType::Float, &chargingDuration, 0.0f, 10.0f});
    fields.push_back({"Character", InspectorField::FieldType::GameObject, &character});
}

bool Spouts::Init()
{
    return true;
}

void Spouts::Update(float deltaTime)
{
    if (activationState == ACTIVATION_STATE::SLEEPING)
    {
        if (character == nullptr) return;
        SphereColliderComponent* damageCollider = parent->GetComponent<SphereColliderComponent*>();
        damageCollider->SetEnabled(false);
        float distance = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
        if (distance <= activationRange)
        {
            activationState = ACTIVATION_STATE::CHARGING;
            chargingTimer   = 0.0f;
        }
    }
    else if (activationState == ACTIVATION_STATE::CHARGING)
    {
        chargingTimer += deltaTime;
        if (chargingTimer >= chargingDuration)
        {
            activationState = ACTIVATION_STATE::DAMAGING;
        }
    }
    else if (activationState == ACTIVATION_STATE::DAMAGING)
    {
        float distance  = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
        activationState = ACTIVATION_STATE::SLEEPING;
        if (distance <= activationRange)
        {
            SphereColliderComponent* damageCollider = parent->GetComponent<SphereColliderComponent*>();

            if (damageCollider)
            {
                damageCollider->SetEnabled(true);
            }
        }
    }
}