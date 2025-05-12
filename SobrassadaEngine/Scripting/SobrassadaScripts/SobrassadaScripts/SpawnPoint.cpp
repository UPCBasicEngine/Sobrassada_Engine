#include "pch.h"

#include "SpawnPoint.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"

SpawnPoint::SpawnPoint(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Set only once", InspectorField::FieldType::Bool, &isOneUse});
}

bool SpawnPoint::Init()
{
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    if (!player)
    {
        GLOG("[WARNING] SpawnPoint: No player found by the name '%s'", playerName.c_str());
        return false;
    }

    return true;
}

void SpawnPoint::OnCollision(GameObject* otherObject, const float3& collisionNormal)
{
    if (otherObject != player) return;

    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            playerScript->SetSpawnPosition(parent->GetPosition());
            if (isOneUse)
            {
                if (CubeColliderComponent* collider = parent->GetComponent<CubeColliderComponent*>())
                    collider->SetEnabled(false);
                parent->SetEnabled(false);
            }
        }
    }
}