#include "pch.h"

#include "MiniFireball.h"
#include "GameObject.h"
#include "ScriptComponent.h" 
#include "Character.h"  

#include <algorithm>

bool MiniFireball::Init()
{
    lifeTimer = 0.f;

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
    if (lifeTimer >= life) parent->SetEnabled(false);

    if (shadow && parent->IsEnabled())
    {
        //local position respect to the firetrap
        float3 pLocal      = parent->GetLocalTransform().TranslatePart();

        // shadow movment
        float3 localOffset = float3(0.f, -pLocal.y, 0.f);
        float3 scale       = baseScale;
        float4x4 tf        = float4x4::FromTRS(localOffset, float3x3::identity, scale);
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
