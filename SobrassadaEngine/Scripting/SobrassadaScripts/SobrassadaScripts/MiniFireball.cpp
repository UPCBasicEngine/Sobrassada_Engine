#include "pch.h"

#include "MiniFireball.h"
#include "Character.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include <algorithm>

bool MiniFireball::Init()
{
    lifeTimer = 0.f;

    // Apply initial random rotation if specified
    if (initialRotation.x != 0.f || initialRotation.y != 0.f || initialRotation.z != 0.f)
    {
        currentRotation           = initialRotation;

        // Apply initial rotation by multiplying current transform
        float4x4 currentTransform = parent->GetLocalTransform();
        float4x4 rotX             = float4x4::RotateX(initialRotation.x * DEGREE_RAD_CONV);
        float4x4 rotY             = float4x4::RotateY(initialRotation.y * DEGREE_RAD_CONV);
        float4x4 rotZ             = float4x4::RotateZ(initialRotation.z * DEGREE_RAD_CONV);

        parent->SetLocalTransform(currentTransform * rotX * rotY * rotZ);
    }

    if (!parent->GetChildren().empty())
    {
        shadow = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
        if (shadow)
        {
            baseScale = shadow->GetScale();
            shadow->SetEnabled(true);
        }
    }
    return true;
}

void MiniFireball::Update(float deltaTime)
{
    lifeTimer += deltaTime;
    if (lifeTimer >= life)
    {
        parent->SetEnabled(false);
        return;
    }

    // Apply continuous rotation incrementally
    if (rotationSpeed.x != 0.f || rotationSpeed.y != 0.f || rotationSpeed.z != 0.f)
    {
        float3 deltaRot = rotationSpeed * deltaTime * DEGREE_RAD_CONV;

        // Apply rotation as incremental transforms
        float4x4 spin   = float4x4::RotateX(deltaRot.x) * float4x4::RotateY(deltaRot.y) * float4x4::RotateZ(deltaRot.z);

        // Multiply current transform by the rotation
        parent->SetLocalTransform(parent->GetLocalTransform() * spin);
    }

    if (shadow && parent->IsEnabled())
    {
        // Shadow movement
        float3 pLocal      = parent->GetLocalTransform().TranslatePart();
        float3 localOffset = float3(0.f, -pLocal.y, 0.f);
        float4x4 tf        = float4x4::FromTRS(localOffset, float3x3::identity, baseScale);
        shadow->SetLocalTransform(tf);
    }
}

void MiniFireball::OnCollision(GameObject* other, const float3 normal, ColliderLayer layer)
{
    if (auto* sc = other->GetComponent<ScriptComponent*>())
        if (auto* character = sc->GetScriptByType<Character>()) character->TakeDamage(damage);

    parent->SetEnabled(false);
    if (shadow) shadow->SetEnabled(false);
}