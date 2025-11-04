#include "pch.h"

#include "Character.h"
#include "GameObject.h"
#include "Math/Quat.h"
#include "ParticleSystemComponent.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "Spouts.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"
#include "Wwise_IDs.h"

Spouts::Spouts(GameObject* parent) : Script(parent)
{
    fields.push_back({"Enable Rune", InspectorField::FieldType::Bool, &enableRune});
    fields.push_back({"Enable Particles Rune", InspectorField::FieldType::Bool, &enableParticlesRune});
    fields.push_back({"Boss Controlled", InspectorField::FieldType::Bool, &bossControlled});
    fields.push_back({"Activation Range", InspectorField::FieldType::Float, &activationRange, 0.0f, 100.0f});
    fields.push_back({"Damage", InspectorField::FieldType::Int, &damage, 0, 5});
    fields.push_back({"Charging Duration", InspectorField::FieldType::Float, &chargingDuration, 0.01f, 10.0f});
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
    fields.push_back({"Explosion Duration", InspectorField::FieldType::Float, &explosionDuration, 0.01f, 0.5f});
    fields.push_back({"Rotation Speed Cylinder", InspectorField::FieldType::Float, &rotationCylinder, 0.0f, 180.0f});
    fields.push_back(
        {"Rotation Speed Tornado After", InspectorField::FieldType::Float, &rotationSpeedTornadoAfter, 0.0f, 180.0f}
    );
    fields.push_back({"Water Spout Duration", InspectorField::FieldType::Float, &spoutWaterTimer, 0.01f, 100.0f});
    fields.push_back({"Damage Cooldown", InspectorField::FieldType::Float, &damageCooldown, 0.0f, 5.0f});
    fields.push_back({"Character", InspectorField::FieldType::GameObject, &character});
    fields.push_back(
        {"Trigger Spout",
         [this](Script* self)
         {
             //GLOG("Triggering spout");
             ForceActivate();
         }}
    );
}

bool Spouts::Init()
{
    whiteWaves           = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
    tornadoWater         = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
    blueWaves            = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[2]);
    waterMesh            = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[3]);
    explosion            = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[4]);
    particleGOB          = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[5]);
    rune                 = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[6]);
    particleGOT          = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[7]);
    runeParticlesGO      = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[8]);

    damageCollider       = parent->GetComponent<SphereColliderComponent*>();

    shaderWaterMesh      = waterMesh->GetComponent<MeshComponent*>();
    shaderScript         = waterMesh->GetComponent<ShaderScriptComponent*>();

    shaderExplosionMesh  = explosion->GetComponent<MeshComponent*>();
    explosionScript      = explosion->GetComponent<ShaderScriptComponent*>();

    shaderwhiteWavesMesh = whiteWaves->GetComponent<MeshComponent*>();
    whiteWavesScript     = whiteWaves->GetComponent<ShaderScriptComponent*>();

    particles_bot        = particleGOB->GetComponent<ParticleSystemComponent*>();
    particles_top        = particleGOT->GetComponent<ParticleSystemComponent*>();
    if (runeParticlesGO) runeParticles = runeParticlesGO->GetComponent<ParticleSystemComponent*>();
    audio = parent->GetComponent<AudioSourceComponent*>();

    return true;
}

void Spouts::Update(float deltaTime)
{
    if (damageGiven)
    {
        damageTimer += deltaTime;
        if (damageTimer >= damageCooldown)
        {
            damageGiven = false;
            damageCollider->SetEnabled(true);
        }
    }

    if (activationState == ACTIVATION_STATE::SLEEPING)
    {
        if (character == nullptr) return;

        damageCollider->SetEnabled(false);

        if (!bossControlled)
        {
            if (enableRune) rune->SetEnabled(true);
            if (enableParticlesRune) runeParticlesGO->SetEnabled(true);

            float distance = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
            if (distance <= activationRange)
            {
                if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_WATER_SPOUTS);
                activationState = ACTIVATION_STATE::CHARGING;
                rune->SetEnabled(false);
                runeParticlesGO->SetEnabled(false);
                tornadoWater->SetEnabled(true);
                chargingTimer = 0.0f;
            }
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
        if (!bossControlled)
        {
            float distance = character->GetGlobalTransform().TranslatePart().DistanceSq(parent->GetPosition());
        }

        waterMesh->SetEnabled(true);
        shaderScript->SetScriptEnabled("MovingUVTransparent", true);
        shaderWaterMesh->SetEnabled(false);
        whiteWaves->SetEnabled(true);
        blueWaves->SetEnabled(true);

        whiteWavesScript->SetScriptEnabled("MovingUVTransparent", true);
        shaderwhiteWavesMesh->SetEnabled(false);

        explosion->SetEnabled(true);

        explosionScript->SetScriptEnabled("MovingUVTransparent", true);
        shaderExplosionMesh->SetEnabled(false);

        if (!bossControlled)
        {
            particleGOB->SetEnabled(true);
            particles_bot->Init();
        }
        particleGOT->SetEnabled(true);
        particles_top->Init();

        // Tornado Water
        float3 translation, scale;
        Quat rotation;

        tornadoWater->GetLocalTransform().Decompose(translation, rotation, scale);
        scale                     = float3(maxScaleTornado);
        float4x4 transformTornado = float4x4::FromTRS(translation, rotation, scale);
        tornadoWater->SetLocalTransform(transformTornado);

        activationState = ACTIVATION_STATE::COOLDOWN;
        chargingTimer   = 0.0f;

        damageCollider->SetEnabled(true);
    }
    else if (activationState == ACTIVATION_STATE::COOLDOWN)
    {
        // White Water Waves
        float4x4 whiteWavesTransform = whiteWaves->GetLocalTransform();
        whiteWavesTransform          = whiteWavesTransform * float4x4::RotateY(rotationSpeedWhiteWaves * deltaTime);
        whiteWaves->SetLocalTransform(whiteWavesTransform);

        // Tornado Water
        float4x4 transformTornado = tornadoWater->GetLocalTransform();
        transformTornado          = transformTornado * float4x4::RotateZ(rotationSpeedTornadoAfter * deltaTime);
        tornadoWater->SetLocalTransform(transformTornado);

        // Blue Water Waves
        float4x4 blueWavesTransform = blueWaves->GetLocalTransform();
        blueWavesTransform          = blueWavesTransform * float4x4::RotateY(rotationSpeedBlueWaves * deltaTime);
        blueWaves->SetLocalTransform(blueWavesTransform);

        float4x4 waterMeshTransform = waterMesh->GetLocalTransform();
        waterMeshTransform          = waterMeshTransform * float4x4::RotateY(rotationCylinder * deltaTime);
        waterMesh->SetLocalTransform(waterMeshTransform);

        chargingTimer += deltaTime;

        if (chargingTimer >= explosionDuration) explosion->SetEnabled(false);
        if (chargingTimer >= spoutWaterTimer)
        {
            whiteWaves->SetEnabled(false);
            tornadoWater->SetEnabled(false);
            waterMesh->SetEnabled(false);
            blueWaves->SetEnabled(false);
            particleGOB->SetEnabled(false);
            particleGOT->SetEnabled(false);
            shaderScript->ResetScript("MovingUVTransparent");

            damageCollider->SetEnabled(false);

            if (chargingTimer >= spoutWaterTimer + 2.0f) activationState = ACTIVATION_STATE::SLEEPING;
        }
    }
}

void Spouts::DisableCollider()
{
    if (bossControlled) return;
    damageCollider->SetEnabled(false);
    damageTimer = 0.0f;
    damageGiven = true;
}

void Spouts::ForceActivate()
{
    //GLOG("Force Activation");

    ResetUVs();
    damageCollider->SetEnabled(false);
    if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_WATER_SPOUTS);
    activationState = ACTIVATION_STATE::CHARGING;
    if (rune) rune->SetEnabled(false);
    if (tornadoWater) tornadoWater->SetEnabled(true);
    chargingTimer = 0.0f;
}

void Spouts::ForceDeactivate()
{
    if (whiteWaves) whiteWaves->SetEnabled(false);
    if (tornadoWater) tornadoWater->SetEnabled(false);
    if (waterMesh) waterMesh->SetEnabled(false);
    if (blueWaves) blueWaves->SetEnabled(false);
    if (explosion) explosion->SetEnabled(false);
    if (particleGOB) particleGOB->SetEnabled(false);
    if (particleGOT) particleGOT->SetEnabled(false);

    if (shaderScript) shaderScript->ResetScript("MovingUVTransparent");
    if (whiteWavesScript) whiteWavesScript->ResetScript("MovingUVTransparent");
    if (explosionScript) explosionScript->ResetScript("MovingUVTransparent");

    if (damageCollider) damageCollider->SetEnabled(false);
    damageGiven     = false;
    damageTimer     = 0.0f;

    chargingTimer   = 0.0f;
    activationState = ACTIVATION_STATE::SLEEPING;
}

void Spouts::ResetUVs()
{
    if (shaderScript) shaderScript->ResetScript("MovingUVTransparent");
    if (whiteWavesScript) whiteWavesScript->ResetScript("MovingUVTransparent");
    if (explosionScript) explosionScript->ResetScript("MovingUVTransparent");

    // Re-enable them again immediately to restart animation
    if (shaderScript) shaderScript->SetScriptEnabled("MovingUVTransparent", true);
    if (whiteWavesScript) whiteWavesScript->SetScriptEnabled("MovingUVTransparent", true);
    if (explosionScript) explosionScript->SetScriptEnabled("MovingUVTransparent", true);
}