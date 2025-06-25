#include "pch.h"

#include "GameObject.h"
#include "Mushroom.h"
#include "ScriptComponent.h"
#include "Standalone/MeshComponent.h"
#include "Standalone/Physics/SphereColliderComponent.h"

Mushroom::Mushroom(GameObject* parent) : Script(parent)
{
}

bool Mushroom::Init()
{
    mushroom = parent->GetComponent<MeshComponent*>();
    if (!mushroom) GLOG("[WARNING] Mushroom without mesh component.");

    collider = parent->GetComponent<SphereColliderComponent*>();
    if (!collider) GLOG("[WARNING] Mushroom without sphere collider component.");

    return true;
}

void Mushroom::Update(float deltaTime)
{
    if (!mushroom || !collider) return;
}

void Mushroom::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    ScriptComponent* playerScript = otherObject->GetComponent<ScriptComponent*>();
    if (playerScript)
    {
        // TODO: some vfx and sfx
    }
}

bool Mushroom::IsReady() const
{
    if (mushroom && mushroom->GetEnabled() && collider && collider->GetEnabled()) return true;

    return false;
}

void Mushroom::Disable()
{
    parent->SetEnabled(false);
}
