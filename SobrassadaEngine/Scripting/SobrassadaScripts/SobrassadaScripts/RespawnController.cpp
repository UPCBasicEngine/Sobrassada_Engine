#include "pch.h"

#include "Globals.h"
#include "RespawnController.h"

#include "CuChulainn.h"
#include "GameObject.h"

#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"


RespawnController::RespawnController(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Respawn timer", InspectorField::FieldType::Float, &respawnInit});
    fields.push_back({"Is player dead", InspectorField::FieldType::Bool, &isPlayerDead});
}
bool RespawnController::Init()
{
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
 
    respawnTimer = respawnInit;
    return true;
}

void RespawnController::Update(float deltaTime)
{
    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            if (playerScript->IsDead())
            {
                respawnTimer -= deltaTime;

                if (respawnTimer <= 0)
                {
                    respawnTimer = 0;
                    playerScript->SetDeath(false);
                    playerScript->SetHealth(3);
                    player->SetEnabled(true);
                }
            }
        }
       
    }
    
   

}

void RespawnController::Respawn()
{
    cuState->Respawn();
}
