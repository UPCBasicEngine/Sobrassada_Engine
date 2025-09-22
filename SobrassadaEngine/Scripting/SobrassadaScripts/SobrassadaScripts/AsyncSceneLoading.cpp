#include "pch.h"

#include "AsyncSceneLoading.h"

#include "GameObject.h"
#include "ProjectModule.h"
#include "Standalone/VideoComponent.h"

AsyncSceneLoading::AsyncSceneLoading(GameObject* parent) : Script(parent)
{
    fields.push_back({"Use async loading", InspectorField::FieldType::Bool, &useAsyncLoading});
    fields.emplace_back("Target Scene Name", InspectorField::FieldType::InputText, &targetSceneName);
}

bool AsyncSceneLoading::Init()
{
    videoComponent = parent->GetComponent<VideoComponent*>();
    if (!videoComponent)
    {
        isSetupCorrectly = false;
        GLOG("No video component found")
        return false;
    }

    fullScenePath =
        AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH + targetSceneName + SCENE_EXTENSION;

    videoComponent->Play();
    if (useAsyncLoading) AppEngine->GetSceneModule()->InitAsyncScenePreLoad(fullScenePath);

    return true;
}

void AsyncSceneLoading::Update(float deltaTime)
{
    if (!isSetupCorrectly) return;

    if (!videoComponent->IsPlaying() && (!useAsyncLoading || AppEngine->GetSceneModule()->IsAsyncSceneLoaded()))
        AppEngine->GetSceneModule()->RequestSceneLoad(fullScenePath);
}