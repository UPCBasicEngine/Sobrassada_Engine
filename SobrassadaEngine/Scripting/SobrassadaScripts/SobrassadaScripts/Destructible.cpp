

#include "pch.h"

#include "Application.h"
#include "Destructible.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "Globals.h"
#include "ParticleSystemComponent.h"
#include "ResourceStateMachine.h"
#include "ScriptComponent.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/MeshComponent.h"
#include "Wwise_IDs.h"
#include "Standalone/Physics/CapsuleColliderComponent.h"

#include <Math/MathFunc.h>
#include <random>

Destructible::Destructible(GameObject* parent): Script(parent)
{
    fields.emplace_back("Destroyed mesh", InspectorField::FieldType::InputText, &destroyedMeshName);
    fields.emplace_back(
        "Destruction particle system", InspectorField::FieldType::InputText, &destructionParticleSystemName
    );
    fields.emplace_back("Time until mesh switch (s)", InspectorField::FieldType::Float, &destructionSpawnDelay);
    fields.emplace_back("0: Vase, 1: Box, 2: Crystal", InspectorField::FieldType::Int, &destructibleTypeIndex, 0, 2);
    fields.emplace_back("Time until destroyed mesh disappears (s)", InspectorField::FieldType::Float, &destructionDespawnDelay, 0, 10);
    fields.emplace_back("Time for rubble to disappear (s)", InspectorField::FieldType::Float, &timeToDisappear, 0, 10);
}

bool Destructible::Init()
{
    // GLOG("Initiating Soldier");

    ValidateSetup();

    currentState = DestructibleStates::NORMAL;

    if (isSetupCorrectly)
    {
        type = static_cast<DestructibleType>(destructibleTypeIndex);
        destroyedMesh->SetEnabled(false);
        destructionSmoke->SetEnabled(false);
        origDestroyedMeshHeight = destroyedMesh->GetLocalPostition().y;
    }

    return isSetupCorrectly;
}

void Destructible::Update(float deltaTime)
{
    if (isSetupCorrectly && currentState == DestructibleStates::DESTROYED)
    {
        if (isSimulating)
        {
            destructionSpawnDelayCounter -= deltaTime;
            if (destructionSpawnDelayCounter <= 0)
            {
                destroyedMesh->SetEnabled(true);

                isSimulating = false;

                destructionDespawnDelayCounter = destructionDespawnDelay;
                isWaitingToDespawn = true;
            }
        } else if (isWaitingToDespawn)
        {
            destructionDespawnDelayCounter -= deltaTime;
            if (destructionDespawnDelayCounter <= 0)
            {
                isWaitingToDespawn = false;

                isDespawning = true;
            }
        } else if (isDespawning)
        {
            const float3& localPosition = destroyedMesh->GetPosition();
            destroyedMesh->SetLocalPosition(
                float3(localPosition.x, origDestroyedMeshHeight -
                    Pow(1024, (1.f / timeToDisappear) * disappearCounter - 1), localPosition.z)
            );
            disappearCounter += deltaTime;
            if (disappearCounter >= timeToDisappear)
            {
                parent->SetEnabled(false);
                currentState = DestructibleStates::DESPAWNED;
            }
        }
        
    }
}

void Destructible::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (isSetupCorrectly && currentState == DestructibleStates::NORMAL)
    {
        // Don´t accept the players hitbox as a hit
        if (otherObject->GetComponent<ScriptComponent*>() != nullptr &&
            otherObject->GetComponent<ScriptComponent*>()->GetScriptByType<CuChulainn>() != nullptr) return;
        
        currentState                 = DestructibleStates::DESTROYED;
        destructionSpawnDelayCounter = destructionSpawnDelay;

        destructionSmoke->SetEnabled(true);
        destructionSmoke->Init();

        switch (type)
        {
        case DestructibleType::VASE:
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_BREAK_02);
            break;
        case DestructibleType::BOX:
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_BREAK_01);
            break;
        case DestructibleType::CRYSTAL:
            audioComp->EmitEvent(AK::EVENTS::PLAY_SFX_BREAK_03);
            break;
        }
        
        playerScript->OnObjectDestroyed();

        parent->GetComponent<CapsuleColliderComponent*>()->SetEnabled(false);
        defaultMesh->SetEnabled(false);

        isSimulating = true;
    }
}

void Destructible::ValidateSetup()
{
    isSetupCorrectly = true;

    defaultMesh      = parent->GetComponent<MeshComponent*>();
    if (defaultMesh == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Default mesh component not found")
        return;
    }

    // Validate children game objects
    for (UID childUID : parent->GetChildren())
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child == nullptr)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Child game object is nullptr")
            return;
        }

        if (child->GetName() == destroyedMeshName)
        {
            destroyedMesh = child;
        }
        else if (child->GetName() == destructionParticleSystemName)
        {
            destructionSmoke = child->GetComponent<ParticleSystemComponent*>();
        }
    }

    if (destroyedMesh == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Destroyed mesh for destruction not found")
        return;
    }

    if (destructionSmoke == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Particle system for destruction not found")
        return;
    }

    // Validate type input
    if (destructibleTypeIndex < 0 || destructibleTypeIndex > 2)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Type input needs to be on of [0, 1, 2]")
        return;
    }

    audioComp = parent->GetComponent<AudioSourceComponent*>();
    if (audioComp == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Script parent does not contain an audio component")
        return;
    }
}
