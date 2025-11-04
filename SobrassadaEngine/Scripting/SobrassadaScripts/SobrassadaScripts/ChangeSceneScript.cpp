#include "pch.h"

#include "ChangeSceneScript.h"
#include "CuChulainn.h"
#include "FileSystem/FileSystem.h"
#include "GameObject.h"
#include "Globals.h"
#include "ProjectModule.h"
#include "SavePlayerData.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "UIFadeInOut.h"
#include "Standalone/Physics/CubeColliderComponent.h"

ChangeSceneScript::ChangeSceneScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Target Scene Name", InspectorField::FieldType::InputText, &targetSceneName});
    
    fields.emplace_back("Fade out game object", InspectorField::FieldType::InputText, &fadeOutGameObjectName);
}

bool ChangeSceneScript::Init()
{
    player        = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    scenesPath    = AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH;
    fullScenePath = scenesPath + targetSceneName + SCENE_EXTENSION;

    if (!player)
    {
        GLOG("[WARNING] ChangeSceneScript: No player found by the name '%s'", playerName.c_str());
        return false;
    }

    GameObject* fadeOutGameObject = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(fadeOutGameObjectName);
    if (fadeOutGameObject != nullptr && fadeOutGameObject->GetComponent<ShaderScriptComponent*>() != nullptr)
        optionalFadeOutScript = fadeOutGameObject->GetComponent<ShaderScriptComponent*>()->GetScriptByType<UIFadeInOut>();
    return true;
}

void ChangeSceneScript::Update(float deltaTime)
{
    if (!changeSceneTriggered) return;
    
    if (timer <= 0)
    {
        if (ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>(); scriptComp != nullptr)
        {
            const CuChulainn* mcScript = scriptComp->GetScriptByType<CuChulainn>();
            if (mcScript != nullptr)
            {
                //GLOG("Processing scene change request to: %s", targetSceneName.c_str())

                const std::string projectPath = AppEngine->GetProjectModule()->GetLoadedProjectPath();
                const std::string savePath    = SavePlayerData::MakeSavePath(projectPath);

                PlayerState playerState;
                mcScript->ExportState(playerState);
                SavePlayerData::SavePlayerToFile(playerState, savePath);

                AppEngine->GetSceneModule()->RequestSceneLoad(fullScenePath);
            }
        }
        changeSceneTriggered = false;
    } else
        timer -= deltaTime;
    
}

void ChangeSceneScript::OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer)
{
    if (otherObject != player || changeSceneTriggered) return;

    if (optionalFadeOutScript != nullptr)
    {
        optionalFadeOutScript->FadeIn();
        timer = optionalFadeOutScript->GetFadeInDuration();
    }

    changeSceneTriggered = true;
}