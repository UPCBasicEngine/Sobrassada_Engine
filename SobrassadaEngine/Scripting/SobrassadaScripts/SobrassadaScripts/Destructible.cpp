

#include "pch.h"

#include "Application.h"
#include "Destructible.h"
#include "GameObject.h"
#include "Globals.h"
#include "ParticleSystemComponent.h"
#include "ResourceStateMachine.h"
#include "Standalone/AIAgentComponent.h"
#include "Standalone/MeshComponent.h"
#include <Math/MathFunc.h>
#include <random>

Destructible::Destructible(GameObject* parent)
    : Character(parent, 1, -1, -1, -1, -1, -1, -1, -1, CharacterType::Destructible)
{
    fields.emplace_back("Destroyed mesh", InspectorField::FieldType::InputText, &destroyedMeshName);
    fields.emplace_back(
        "Destruction particle system", InspectorField::FieldType::InputText, &destructionParticleSystemName
    );
    fields.emplace_back("Time for rubble to disappear (s)", InspectorField::FieldType::Float, &timeToDisappear);
}

bool Destructible::Init()
{
    // GLOG("Initiating Soldier");

    ValidateSetup();

    currentState = DestructibleStates::NORMAL;

    Character::Init();

    if (isSetupCorrectly)
    {
        destroyedMesh->SetEnabled(false);
        destructionSmoke->SetEnabled(false);
    }

    return isSetupCorrectly;
}

void Destructible::Update(float deltaTime)
{
    if (!isDead && isSetupCorrectly && currentState == DestructibleStates::DESTROYED)
    {
        const float3& localPosition = destroyedMesh->GetPosition();
        destroyedMesh->SetLocalPosition(
            float3(localPosition.x, -Pow(1024, (1.f / timeToDisappear) * disappearCounter - 1), localPosition.z)
        );
        disappearCounter += deltaTime;
        if (disappearCounter >= timeToDisappear)
        {
            isDead = true;
            destroyedMesh->SetEnabled(false);
            destructionSmoke->SetEnabled(false);
        }
    }
}

void Destructible::OnDeath()
{
    if (isSetupCorrectly)
    {
        currentState = DestructibleStates::DESTROYED;

        defaultMesh->SetEnabled(false);
        destroyedMesh->SetEnabled(true);
        destructionSmoke->SetEnabled(true);
        destructionSmoke->Init();

        isDead = false;
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

    if (destructionSmoke == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Particle system for destruction not found")
        return;
    }
}
