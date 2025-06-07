#include "pch.h"

#include "PauseMenuScript.h"
#include "Application.h"
#include "GameObject.h"
#include "InputModule.h"
#include "Scene.h"
#include "SceneModule.h"


bool PauseMenuScript::Init()
{
    return true;
}

void PauseMenuScript::Update(float deltaTime)
{
    const KeyState* keys           = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* gamepadButtons = AppEngine->GetInputModule()->GetControllerButtons();

    if (!cachedTarget)
    {
        const auto& allGameObjects = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();

        for (const auto& [uid, gameObject] : allGameObjects)
        {
            if (gameObject && gameObject->GetName() == panelToShowName)
            {
                cachedTarget = gameObject;
                break;
            }
        }
    }

    if (cachedTarget &&
        (keys[SDL_SCANCODE_ESCAPE] == KEY_DOWN || gamepadButtons[SDL_CONTROLLER_BUTTON_START] == KEY_DOWN))
    {
        bool newState = !cachedTarget->IsEnabled();
        cachedTarget->SetEnabledRecursive(newState);
    }
}


void PauseMenuScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("PanelToShow", rapidjson::Value(panelToShowName.c_str(), allocator), allocator);
}

void PauseMenuScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("PanelToShow") && initialState["PanelToShow"].IsString())
    {
        panelToShowName = initialState["PanelToShow"].GetString();
    }
}
