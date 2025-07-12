#include "pch.h"

#include "Spouts.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Character.h"

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
        float distance = character->GetPosition().DistanceSq(parent->GetPosition());
        if (distance <= activationRange * activationRange)
        {
            activationState = ACTIVATION_STATE::CHARGING;
            chargingTimer       = 0.0f;
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
        float distance = character->GetPosition().DistanceSq(parent->GetPosition());
        if (distance <= activationRange * activationRange)
        {
            // Activar Collider y hacer esto en OnCollision
            ScriptComponent* script = character->GetComponent<ScriptComponent*>();
            if (script && script->GetScriptByType<Character>()) return;

            Character* characterScript = script->GetScriptByType<Character>();
            characterScript->TakeDamage(damage);
        }
        activationState = ACTIVATION_STATE::SLEEPING;
    }
}