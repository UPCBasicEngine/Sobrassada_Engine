#include "pch.h"

#include "Application.h"
#include "GameObject.h"
#include "InputModule.h"
#include "MainMenuSelectorScript.h"
#include "Scene.h"
#include "SceneModule.h"
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
    const KeyState* keys = AppEngine->GetInputModule()->GetKeyboard();

    if (keys[SDL_SCANCODE_DOWN] == KEY_DOWN)
    {
        selectedIndex = (selectedIndex + 1) % menuItems.size();
        UpdateSelection();
    }
    else if (keys[SDL_SCANCODE_UP] == KEY_DOWN)
    {
        selectedIndex = (selectedIndex - 1 + menuItems.size()) % static_cast<int>(menuItems.size());
        UpdateSelection();
    }

    if (keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN)
    {
        GameObject* selectedItem = menuItems[selectedIndex];

        ButtonComponent* button  = selectedItem->GetComponent<ButtonComponent*>();
        if (button) button->OnClick();
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
