#include "pch.h"

#include "AsyncSceneLoading.h"

#include "ChangeSceneScript.h"
#include "GameObject.h"
#include "ScriptComponent.h"
#include "Standalone/VideoComponent.h"

bool AsyncSceneLoading::Init()
{
    videoComponent = parent->GetComponent<VideoComponent*>();
    if (!videoComponent)
    {
        isSetupCorrectly = false;
        GLOG("No video component found")
        return false;
    }

    changeSceneScript = parent->GetComponent<ScriptComponent*>()->GetScriptByType<ChangeSceneScript>();
    if (!changeSceneScript)
    {
        isSetupCorrectly = false;
        GLOG("No change scene script found")
        return false;
    }

    AppEngine->GetSceneModule()->InitAsyncScenePreLoad(changeSceneScript->GetFullScenePath());
    videoComponent->Play();

    return true;
}

void AsyncSceneLoading::Update(float deltaTime)
{
    if (!isSetupCorrectly) return;

    if (!videoComponent->IsPlaying() && AppEngine->GetSceneModule()->IsAsyncSceneLoaded())
        changeSceneScript->SwitchScene();
        
}

void AsyncSceneLoading::OnDestroy()
{
   
}