#include "pch.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "InputModule.h"
#include "PressAnyKeyScript.h"
#include "Scene.h"
#include "SceneModule.h"
#include "GameUIModule.h"
#include <imgui.h>


PressAnyKeyScript::PressAnyKeyScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Next GameObject to Show", InspectorField::FieldType::InputText, &nextGameObjectName});
}

bool PressAnyKeyScript::Init()
{
    if (!parent)
    {
        return false;
    }

    return true;
}

void PressAnyKeyScript::Update(float deltaTime)
{
    if (!parent || !parent->IsEnabled()) return;

    const KeyState* keys           = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* gamepadButtons = AppEngine->GetInputModule()->GetControllerButtons();

    bool keyPressed                = keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN ||
                      gamepadButtons[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN;

    if (AppEngine->GetGameUIModule()->HasShownIntroScreen() || keyPressed)
    {
        AppEngine->GetGameUIModule()->SetIntroScreenShown(true);
        parent->SetEnabled(false);

        const auto& gameObjects = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();
        for (const auto& [uid, go] : gameObjects)
        {
            if (go && go->GetName() == nextGameObjectName)
            {
                go->SetEnabledRecursive(true);
                break;
            }
        }
    }
}
