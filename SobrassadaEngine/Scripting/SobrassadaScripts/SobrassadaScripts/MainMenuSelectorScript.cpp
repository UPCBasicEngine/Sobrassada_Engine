#include "pch.h"

#include "Application.h"
#include "GameObject.h"
#include "InputModule.h"
#include "MainMenuSelectorScript.h"
#include "Scene.h"
#include "SceneModule.h"
#include "GameTimer.h"
#include "Standalone/UI/ButtonComponent.h"

bool MainMenuSelectorScript::Init()
{
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

    static bool stickMoved         = false;

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
            UID parentUID        = selectedItem->GetParent();
            GameObject* parentGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parentUID);
            if (parentGO)
            {
                parentGO->SetEnabledRecursive(false);
            }

            GameTimer* timer = AppEngine->GetGameTimer();
            if (timer && timer->IsPaused())
            {
                timer->TogglePause();
            }
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
