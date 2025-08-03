#include "pch.h"

#include "Character.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Spouts.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

Spouts::Spouts(GameObject* parent) : Script(parent)
{
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &activationRange, 0.0f, 100.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 5});
    fields.push_back({"Charging Duration", InspectorField::FieldType::Float, &chargingDuration, 0.0f, 10.0f});
    fields.push_back({"Rotation Speed", InspectorField::FieldType::Float, &rotationSpeed, 0.0f, 180.0f});
    fields.push_back({"Character", InspectorField::FieldType::GameObject, &character});
}

bool Spouts::Init()
{
    decal          = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
    waterMesh      = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
    damageCollider = parent->GetComponent<SphereColliderComponent*>();

    shaderMesh     = waterMesh->GetComponent<MeshComponent*>();
    shaderScript   = waterMesh->GetComponent<ShaderScriptComponent*>();

    return true;
}

void Spouts::Update(float deltaTime)
{
    if (activationState == ACTIVATION_STATE::SLEEPING)
    {
        if (character == nullptr) return;

        damageCollider->SetEnabled(false);
        float distance = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
        if (distance <= activationRange)
        {
            activationState = ACTIVATION_STATE::CHARGING;
            decal->SetEnabled(true);
            chargingTimer = 0.0f;
        }
    }
    else if (activationState == ACTIVATION_STATE::CHARGING)
    {
        chargingTimer         += deltaTime;

        float4x4 newTransform  = decal->GetLocalTransform();
        newTransform           = newTransform * float4x4::RotateY(rotationSpeed * deltaTime);
        decal->SetLocalTransform(newTransform);

        if (chargingTimer >= chargingDuration)
        {
            activationState = ACTIVATION_STATE::DAMAGING;
        }
    }
    else if (activationState == ACTIVATION_STATE::DAMAGING)
    {
        float distance = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
        decal->SetEnabled(false);
        waterMesh->SetEnabled(true);
        shaderScript->SetScriptEnabled("MovingUVTransparent", true);
        shaderMesh->SetEnabled(false);

        activationState = ACTIVATION_STATE::COOLDOWN;
        chargingTimer   = 0.0f;
        if (distance <= activationRange)
        {
            damageCollider->SetEnabled(true);
        }
    }
    else if (activationState == ACTIVATION_STATE::COOLDOWN)
    {
        chargingTimer                           += deltaTime;
        if (chargingTimer >= chargingDuration)
        {
            waterMesh->SetEnabled(false);
            shaderScript->ResetScript("MovingUVTransparent");

            damageCollider->SetEnabled(false);
            activationState = ACTIVATION_STATE::SLEEPING;
        }
    }
}