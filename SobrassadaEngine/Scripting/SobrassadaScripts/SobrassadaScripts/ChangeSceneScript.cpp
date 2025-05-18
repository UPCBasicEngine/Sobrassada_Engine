#include "pch.h"

#include "ChangeSceneScript.h"

#include "CuChulainn.h"
#include "GameObject.h"
#include "Scene.h"
#include "Application.h"
#include "SceneModule.h"
#include "ProjectModule.h"
#include "LibraryModule.h"
#include "ScriptComponent.h"
#include "FileSystem.h"
#include "Standalone/Physics/CubeColliderComponent.h"



ChangeSceneScript::ChangeSceneScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Player name", InspectorField::FieldType::InputText, &playerName});
    fields.push_back({"Set only once", InspectorField::FieldType::Bool, &isOneUse});
    fields.push_back({"Scene index", InspectorField::FieldType::Int, &indexScene});
}

bool ChangeSceneScript::Init()
{
    player = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName(playerName);
    scenesPath = App->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH;
    if (!player)
    {
        GLOG("[WARNING] SceneChangePoint: No player found by the name '%s'", playerName.c_str());
        return false;
    }

    return true;
}


void ChangeSceneScript::OnCollision(GameObject* otherObject, const float3& collisionNormal)
{
    if (otherObject != player) return;

    ScriptComponent* scriptComp = player->GetComponent<ScriptComponent*>();
    if (scriptComp)
    {
        CuChulainn* playerScript = scriptComp->GetScriptByType<CuChulainn>();
        if (playerScript)
        {
            //change scene 
            //playerScript->SetSpawnPosition(parent->GetPosition());
            if (FileSystem::Exists(scenesPath.c_str()))
            {
                    FileSystem::GetFilesSorted(scenesPath, filesLoad);

                    for (int i = 0; i < filesLoad.size(); i++)
                    {
                        const std::string& file = filesLoad[i];
                        GLOG("SCENE NAME: %s ", file);
                        if (indexScene == i)
                        {
                            selectedLoad = i;
                            fileName     = file;
                        }
                       


                    }
                
            }
            if (selectedLoad != -1)
            {
                App->GetLibraryModule()->LoadScene(fileName.c_str());
                App->GetSceneModule()->GetInPlayMode();
                playerScript->SetSpawnPosition(parent->GetPosition());
            }
            
            if (isOneUse)
            {
                if (CubeColliderComponent* collider = parent->GetComponent<CubeColliderComponent*>())
                    collider->SetEnabled(false);
                parent->SetEnabled(false);
            }
        }
    }
}
