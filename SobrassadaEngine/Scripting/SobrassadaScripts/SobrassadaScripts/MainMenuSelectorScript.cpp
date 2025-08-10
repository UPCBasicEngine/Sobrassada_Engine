#include "pch.h"

#include "Application.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "GameOverScript.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "MainMenuSelectorScript.h"
#include "PauseMenuScript.h"
#include "ProjectModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/UI/ButtonComponent.h"

bool MainMenuSelectorScript::Init()
{
    menuItems.clear();
    arrowImages.clear();
    selectedIndex                    = 0;

    const std::vector<UID>& children = parent->GetChildren();
    for (UID childUID : children)
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
        if (!child) continue;

        if (child->GetName().find("MenuItem_") != std::string::npos)
        {
            menuItems.push_back(child);

            const std::vector<UID>& itemChildren = child->GetChildren();
            for (UID arrowUID : itemChildren)
            {
                GameObject* arrow = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(arrowUID);
                if (arrow && arrow->GetName().find("Arrow") != std::string::npos)
                {
                    arrowImages.push_back(arrow);
                    arrow->SetEnabled(false);
                }
            }
        }
    }

    Scene* scene     = AppEngine->GetSceneModule()->GetScene();
    GameObject* node = parent; // start with the object that owns this script

    while (node && (!pauseCtrl || !gameOverCtrl))
    {
        ScriptComponent* sc = node->GetComponent<ScriptComponent*>();
        if (sc)
        {
            if (!pauseCtrl) pauseCtrl = sc->GetScriptByType<PauseMenuScript>();
            if (!gameOverCtrl) gameOverCtrl = sc->GetScriptByType<GameOverScript>();
        }
        node = scene->GetGameObjectByUID(node->GetParent());
    }

    UpdateSelection();
    return true;
}

void MainMenuSelectorScript::Update(float deltaTime)
{
    int arrowsEnabled = 0;
    for (GameObject* arrow : arrowImages)
    {
        if (arrow && arrow->IsEnabled())
        {
            ++arrowsEnabled;
        }
    }

    if (arrowsEnabled > 1)
    {
        UpdateSelection();
    }

    const KeyState* keys           = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* gamepadButtons = AppEngine->GetInputModule()->GetControllerButtons();
    const float2& leftStick        = AppEngine->GetInputModule()->GetLeftStick();

    bool moveDown                  = keys[SDL_SCANCODE_DOWN] == KEY_DOWN ||
                    gamepadButtons[SDL_CONTROLLER_BUTTON_DPAD_DOWN] == KEY_DOWN || (leftStick.y > 0.5f && !stickMoved);

    bool moveUp = keys[SDL_SCANCODE_UP] == KEY_DOWN || gamepadButtons[SDL_CONTROLLER_BUTTON_DPAD_UP] == KEY_DOWN ||
                  (leftStick.y < -0.5f && !stickMoved);

    if (moveDown)
    {
        selectedIndex = (selectedIndex + 1) % menuItems.size();
        UpdateSelection();
        stickMoved = true;
    }
    else if (moveUp)
    {
        selectedIndex = (selectedIndex - 1 + menuItems.size()) % static_cast<int>(menuItems.size());
        UpdateSelection();
        stickMoved = true;
    }

    if (fabs(leftStick.y) < 0.3f)
    {
        stickMoved = false;
    }

    if (keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN ||
        gamepadButtons[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        GameObject* selectedItem = menuItems[selectedIndex];

        if (selectedItem->GetName() == "MenuItem_Continue")
        {
            if (gameOverCtrl)
            {
                gameOverCtrl->Close();
                if (playerScript) playerScript->Respawn();
            }
            else if (pauseCtrl)
            {
                pauseCtrl->Close();
            }

            return;
        }

        else if (selectedItem->GetName() == "MenuItem_Menu")
        {
            if (pauseCtrl) pauseCtrl->Close();
            AppEngine->GetSceneModule()->GetScene()->SetStopPlaying(true);
            std::string path =
                AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH + "SCENE_MainMenu.scene";
            AppEngine->GetSceneModule()->RequestSceneLoad(path);
            return;
        }
        else
        {
            ButtonComponent* button = selectedItem->GetComponent<ButtonComponent*>();
            if (button) button->OnClick();
        }
    }
}

void MainMenuSelectorScript::UpdateSelection()
{
    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        for (UID childUID : menuItems[i]->GetChildren())
        {
            GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(childUID);
            if (child && child->GetName().find("Arrow") != std::string::npos)
            {
                child->SetEnabled(i == selectedIndex);
            }
        }
    }
}
