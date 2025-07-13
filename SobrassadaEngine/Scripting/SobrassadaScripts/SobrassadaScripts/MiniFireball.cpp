#include "pch.h"

#include "MiniFireball.h"
#include "GameObject.h"
#include "ScriptComponent.h" 
#include "Character.h"  

bool MiniFireball::Init()
{
    lifeTimer = 0.f;
    return true;
}

void MiniFireball::Update(float dt)
{
    lifeTimer += dt;
    if (lifeTimer >= life) parent->SetEnabled(false);
}

void MiniFireball::OnCollision(GameObject* other, const float3 normal, ColliderLayer layer)
{
    if (auto* sc = other->GetComponent<ScriptComponent*>())
        if (auto* character = sc->GetScriptByType<Character>()) character->TakeDamage(damage);

    parent->SetEnabled(false);
}
