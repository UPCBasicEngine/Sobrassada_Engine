#include "pch.h"

#include "Character.h"
#include "GameObject.h"
#include "Math/Quat.h"
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
    fields.push_back(
        {"Rotation Speed White Waves", InspectorField::FieldType::Float, &rotationSpeedWhiteWaves, 0.0f, 180.0f}
    );
    fields.push_back({"Rotation Speed Tornado", InspectorField::FieldType::Float, &rotationSpeedTornado, 0.0f, 180.0f});
    fields.push_back({"Min Scale Tornado", InspectorField::FieldType::Float, &maxScaleTornado, 5.0f, 20.0f});
    fields.push_back({"Max Scale Tornado", InspectorField::FieldType::Float, &minScaleTornado, 0.0f, 5.0f});
    fields.push_back({"Initial Scale Tornado", InspectorField::FieldType::Float, &initialScaleTornado, 5.0f, 15.0f});
    fields.push_back(
        {"Rotation Speed Blue Waves", InspectorField::FieldType::Float, &rotationSpeedBlueWaves, 0.0f, 180.0f}
    );
    fields.push_back({"Character", InspectorField::FieldType::GameObject, &character});
}

bool Spouts::Init()
{
    whiteWaves     = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
    tornadoWater   = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
    blueWaves      = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[2]);
    explosion      = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[3]);
    waterMesh      = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[4]);
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
            tornadoWater->SetEnabled(true);
            chargingTimer = 0.0f;
        }
    }
    else if (activationState == ACTIVATION_STATE::CHARGING)
    {
        // Tornado Water
        float3 translation, scale;
        Quat rotation;

        tornadoWater->GetLocalTransform().Decompose(translation, rotation, scale);

        Quat deltaRotation         = Quat::RotateY(rotationSpeedTornado * deltaTime);
        rotation                   = deltaRotation * rotation;

        chargingTimer             += deltaTime;

        float t                    = min(chargingTimer / chargingDuration, 1.0f);
        float animatedScale        = initialScaleTornado + (minScaleTornado - initialScaleTornado) * t;

        scale                      = float3(animatedScale);

        float4x4 transformTornado  = float4x4::FromTRS(translation, rotation, scale);
        tornadoWater->SetLocalTransform(transformTornado);

        if (chargingTimer >= chargingDuration)
        {
            activationState = ACTIVATION_STATE::DAMAGING;
        }
    }
    else if (activationState == ACTIVATION_STATE::DAMAGING)
    {
        float distance = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
        waterMesh->SetEnabled(true);
        shaderScript->SetScriptEnabled("MovingUVTransparent", true);
        shaderMesh->SetEnabled(false);
        whiteWaves->SetEnabled(true);
        blueWaves->SetEnabled(true);

        // Tornado Water
        float3 translation, scale;
        Quat rotation;

        tornadoWater->GetLocalTransform().Decompose(translation, rotation, scale);
        scale                     = float3(maxScaleTornado);
        float4x4 transformTornado = float4x4::FromTRS(translation, rotation, scale);
        tornadoWater->SetLocalTransform(transformTornado);

        activationState = ACTIVATION_STATE::COOLDOWN;
        chargingTimer   = 0.0f;
        if (distance <= activationRange)
        {
            damageCollider->SetEnabled(true);
        }
    }
    else if (activationState == ACTIVATION_STATE::COOLDOWN)
    {
        // White Water Waves
        float4x4 whiteWavesTransform = whiteWaves->GetLocalTransform();
        whiteWavesTransform          = whiteWavesTransform * float4x4::RotateY(rotationSpeedWhiteWaves * deltaTime);
        whiteWaves->SetLocalTransform(whiteWavesTransform);

        // Tornado Water
        float4x4 transformTornado = tornadoWater->GetLocalTransform();
        transformTornado          = transformTornado * float4x4::RotateZ(rotationSpeedWhiteWaves * deltaTime);
        tornadoWater->SetLocalTransform(transformTornado);

        // Blue Water Waves
        float4x4 blueWavesTransform = blueWaves->GetLocalTransform();
        blueWavesTransform          = blueWavesTransform * float4x4::RotateY(rotationSpeedBlueWaves * deltaTime);
        blueWaves->SetLocalTransform(blueWavesTransform);

        chargingTimer += deltaTime;

        chargingTimer += deltaTime;
        if (chargingTimer >= chargingDuration)
        {
            whiteWaves->SetEnabled(false);
            tornadoWater->SetEnabled(false);
            waterMesh->SetEnabled(false);
            blueWaves->SetEnabled(false);
            shaderScript->ResetScript("MovingUVTransparent");

            damageCollider->SetEnabled(false);
            activationState = ACTIVATION_STATE::SLEEPING;
        }
    }
}