#include "pch.h"
#include "GameObject.h"
#include "Projectile.h"
#include "ScriptComponent.h"
#include "WallCollision.h"


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

    Projectile* projectile = scriptComp->GetScriptByType<Projectile>();
    if (!projectile)
    {
        GLOG("WallCollision: No projectile script found");
        return;
    }

    GLOG("Wall detected projectile collision!");
    projectile->OnWallHit();
}
