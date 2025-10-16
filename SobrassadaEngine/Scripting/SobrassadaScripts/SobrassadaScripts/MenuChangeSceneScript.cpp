#include "pch.h"

#include "Application.h"
#include "FileSystem/FileSystem.h"
#include "Globals.h"
#include "InputModule.h"
#include "MenuChangeSceneScript.h"
#include "ProjectModule.h"
#include "SceneModule.h"
#include "GameSession.h"

MenuChangeSceneScript::MenuChangeSceneScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Target Scene Name", InspectorField::FieldType::InputText, &targetSceneName});
}

bool MenuChangeSceneScript::Init()
{
    inputDelayFrames = 10;
    scenesPath    = AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH;
    fullScenePath = scenesPath + targetSceneName + SCENE_EXTENSION;
    return true;
}

void MenuChangeSceneScript::Update(float deltaTime)
{
    if (sceneLoaded) return;

    if (inputDelayFrames > 0)
    {
        --inputDelayFrames;
        return;
    }

    const KeyState* keys           = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* gamepadButtons = AppEngine->GetInputModule()->GetControllerButtons();

    if (keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN ||
        gamepadButtons[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        sceneLoaded = true;
        if (targetSceneName == "SCENE_Tutorial") gNewGame = true;

        AppEngine->GetSceneModule()->RequestSceneLoad(fullScenePath);
    }
}
