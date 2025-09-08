
#include "pch.h"
#include "WallCollision.h"
#include "ArcherProjectile.h"
#include "GameObject.h"
#include "ScriptComponent.h"

WallCollision::WallCollision(GameObject* parent) : Script(parent)
{
}

bool WallCollision::Init()
{
    GLOG("INIT INIT INIT WALL COLLISION!!");
    return true;
}

void WallCollision::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{

    GLOG("WallCollision: Something hit the wall - %s", otherObject->GetName().c_str());

    ScriptComponent* scriptComp = otherObject->GetComponent<ScriptComponent*>();
    if (!scriptComp)
    {
        GLOG("WallCollision: No script component found");
        return;
    }

    ArcherProjectile* projectile = scriptComp->GetScriptByType<ArcherProjectile>();
    if (!projectile)
    {
        GLOG("WallCollision: No projectile script found");
        return;
    }

    GLOG("Wall detected projectile collision!");
    projectile->OnWallHit();
}
