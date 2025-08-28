#include "pch.h"

#include "ChangeSceneScript.h"
#include "CuChulainn.h"
#include "FileSystem/FileSystem.h"
#include "GameObject.h"
#include "Globals.h"
#include "ProjectModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/Physics/CubeColliderComponent.h"
#include "SavePlayerData.h"

ChangeSceneScript::ChangeSceneScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Target Scene Name", InspectorField::FieldType::InputText, &targetSceneName});
}

bool ChangeSceneScript::Init()
{
    player        = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    scenesPath    = AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH;
    fullScenePath = scenesPath + targetSceneName + SCENE_EXTENSION;

    if (!player)
    {
        GLOG("[WARNING] ChangeSceneScript: No player found by the name '%s'", playerName.c_str());
        return false;
    }
    return true;
}

void ChangeSceneScript::OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (otherObject != player) return;

    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            GLOG("Processing scene change request to: %s", targetSceneName);

            const std::string projectPath = AppEngine->GetProjectModule()->GetLoadedProjectPath();
            const std::string savePath    = SavePlayerData::MakeSavePath(projectPath);

            PlayerState playerState;
            playerScript->ExportState(playerState);
            SavePlayerData::SavePlayerToFile(playerState, savePath);

            SceneModule* sceneModule = AppEngine->GetSceneModule();

            rapidjson::Document doc;
            if (FileSystem::LoadJSON(fullScenePath.c_str(), doc))
            {
                if (doc.HasMember("Scene") && doc["Scene"].IsObject())
                {
                    sceneModule->LoadScene(doc["Scene"], false);
                    sceneModule->SwitchPlayMode(true);

                    GLOG("Scene change successful!");

                    //Recover player data from JSON
                    Scene* newScene = sceneModule->GetScene();
                    if (!newScene) return;

                    GameObject* newPlayer = newScene->GetGameObjectByName(playerName);
                    if (!newPlayer) return;

                    ScriptComponent* newScriptComp = newPlayer->GetComponent<ScriptComponent*>();
                    if (!newScriptComp) return;

                    CuChulainn* newCuchulainn = newScriptComp->GetScriptByType<CuChulainn>();
                    if (!newCuchulainn) return;

                    PlayerState loadedPlayerState;
                    if (!SavePlayerData::LoadPlayerFromFile(loadedPlayerState, savePath)) return;

                    newCuchulainn->ApplySavedState(loadedPlayerState);
                }
            }

            GLOG("[ERROR] Failed to load scene: %s", targetSceneName);
        }
    }
}