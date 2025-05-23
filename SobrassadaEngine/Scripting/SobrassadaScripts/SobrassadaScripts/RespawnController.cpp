#include "pch.h"
#include "RespawnController.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "Globals.h" 
#include "Scene.h"
#include "SceneModule.h"
#include "FileSystem/FileSystem.h"
#include "ProjectModule.h"
#include "ScriptComponent.h"

RespawnController::RespawnController(GameObject* parent) :  Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Respawn Init", InspectorField::FieldType::Float, &respawnInit});
}
bool RespawnController::Init()
{
    respawnTimer = respawnInit;
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
   
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
                    playerScript->Restart();
                    player->SetEnabled(true);
                    respawnTimer = respawnInit;
               }
            }
        }
    }
}


void RespawnController::Respawn()
{
}
