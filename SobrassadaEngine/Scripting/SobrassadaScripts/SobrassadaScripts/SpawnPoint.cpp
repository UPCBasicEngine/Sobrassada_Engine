#include "pch.h"

#include "SpawnPoint.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/AnimController.h"
#include "Standalone/AnimationComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"

SpawnPoint::SpawnPoint(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Leafs name", InspectorField::FieldType::InputText, &leafsName});
    fields.push_back({"Set only once", InspectorField::FieldType::Bool, &isOneUse});
    fields.push_back({"Set Health for player", InspectorField::FieldType::Int, &setHealth});
}

bool SpawnPoint::Init()
{
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    if (!player)
    {
        isSetupCorrectly = false;
        GLOG("[WARNING] SpawnPoint: No player found by the name '%s'", playerName.c_str());
        return false;
    }

    for (const UID childUID : parent->GetChildren())
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (child == nullptr)
        {
            isSetupCorrectly = false;
            GLOG("[ERROR] Child game object is nullptr")
            return false;
        }

        if (child->GetName() == leafsName) leafs = child;
    }

    if (leafs == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] No child go for leafs found")
        return false;
    }

    leafs->SetEnabled(false);

    return true;
}

void SpawnPoint::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (!isSetupCorrectly || activated || otherObject != player) return;

    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            if (AnimationComponent* animComp = parent->GetComponent<AnimationComponent*>())
            {
                leafs->SetEnabled(true);
                animComp->OnPlay(false, false);
            }
            
            playerScript->SetSpawnPosition(parent->GetPosition());
            playerScript->SetHealth(setHealth);
            if (isOneUse)
            {
                if (CubeColliderComponent* collider = parent->GetComponent<CubeColliderComponent*>())
                    collider->SetEnabled(false);
                parent->SetEnabled(false);
            }
            activated = true;
        }
    }
}