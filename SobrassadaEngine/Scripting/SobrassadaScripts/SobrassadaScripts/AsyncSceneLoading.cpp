#include "pch.h"

#include "AsyncSceneLoading.h"

#include "GameObject.h"
#include "InputModule.h"
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
        AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH + targetSceneName + SCENE_EXTENSION;

    videoComponent->Play();
    if (useAsyncLoading) AppEngine->GetSceneModule()->InitAsyncScenePreLoad(fullScenePath);

    return true;
}

void AsyncSceneLoading::Update(float deltaTime)
{
    if (!isSetupCorrectly) return;

    if ((!videoComponent->IsPlaying() || SkipCutscene()) && (!useAsyncLoading || AppEngine->GetSceneModule()->IsAsyncSceneLoaded()))
        AppEngine->GetSceneModule()->RequestSceneLoad(fullScenePath);
}

bool AsyncSceneLoading::SkipCutscene() const
{
    const InputModule* input   = AppEngine->GetInputModule();
    const KeyState* keyboard   = input->GetKeyboard();
    const KeyState* controller = input->GetControllerButtons();

    if (input->IsUsingKeyboard())
    {
        if (keyboard[SDL_SCANCODE_X] == KEY_REPEAT) return true;
    }
    else
    {
        if (controller[SDL_CONTROLLER_BUTTON_X] == KEY_REPEAT) return true;
        if (controller[SDL_CONTROLLER_BUTTON_A] == KEY_REPEAT) return true;
    }
    return false;
}