#include "pch.h"

#include "Application.h"
#include "GameObject.h"
#include "GameOverScript.h"
#include "GameTimer.h"
#include "Globals.h"
#include "InputModule.h"
#include "MainMenuSelectorScript.h"
#include "PauseMenuScript.h"
#include "ProjectModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/UI/ButtonComponent.h"
#include <algorithm>
#include <cmath>

bool MainMenuSelectorScript::Init()
{
    CachePanel_();

    // find Pause/GameOver controllers walking up from this script's GO
    Scene* scene     = AppEngine->GetSceneModule()->GetScene();
    GameObject* node = parent;
    while (node && (!pauseCtrl || !gameOverCtrl))
    {
        if (auto* sc = node->GetComponent<ScriptComponent*>())
        {
            if (!pauseCtrl) pauseCtrl = sc->GetScriptByType<PauseMenuScript>();
            if (!gameOverCtrl) gameOverCtrl = sc->GetScriptByType<GameOverScript>();
        }
        node = scene->GetGameObjectByUID(node->GetParent());
    }

    if (panelRoot && panelRoot->IsEnabled())
    {
        BuildFromPanel_();
        UpdateSelection_();
        builtOnce = true;
    }

    return true;
}

void MainMenuSelectorScript::CachePanel_()
{
    if (panelRoot) return;

    Scene* scene        = AppEngine->GetSceneModule()->GetScene();
    const auto& objects = scene->GetAllGameObjects();

    // 1) Try exact name from inspector
    for (const auto& [uid, go] : objects)
        if (go && go->GetName() == panelName)
        {
            panelRoot = go;
            break;
        }

    // 2) Fallback: autodetect first GO that has children named "MenuItem_*"
    if (!panelRoot)
    {
        for (const auto& [uid, go] : objects)
        {
            if (!go) continue;
            bool hasMenuChild = false;
            for (UID cid : go->GetChildren())
            {
                GameObject* ch = scene->GetGameObjectByUID(cid);
                if (ch && ch->GetName().find("MenuItem_") != std::string::npos)
                {
                    hasMenuChild = true;
                    break;
                }
            }
            if (hasMenuChild)
            {
                panelRoot = go;
                break;
            }
        }
    }

    GLOG("[SEL] CachePanel -> %s (name=%s)", panelRoot ? "FOUND" : "NOT FOUND", panelName.c_str());
}

void MainMenuSelectorScript::BuildFromPanel_()
{
    if (!panelRoot) return;

    menuItems.clear();
    arrowImages.clear();

    const auto& children = panelRoot->GetChildren();
    for (UID cid : children)
    {
        GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(cid);
        if (!child) continue;

        if (child->GetName().find("MenuItem_") != std::string::npos)
        {
            menuItems.push_back(child);

            for (UID gcid : child->GetChildren())
            {
                GameObject* arrow = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(gcid);
                if (arrow && arrow->GetName().find("Arrow") != std::string::npos) arrowImages.push_back(arrow);
            }
        }
    }

    // turn all arrows off; UpdateSelection_() will light the selected one
    for (auto* a : arrowImages)
        if (a) a->SetEnabled(false);

    GLOG("[SEL] BuildFromPanel: items=%zu arrows=%zu", menuItems.size(), arrowImages.size());
}

void MainMenuSelectorScript::UpdateSelection_()
{
    if (menuItems.empty()) return;
    selectedIndex = std::clamp(selectedIndex, 0, (int)menuItems.size() - 1);

    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        for (UID uid : menuItems[i]->GetChildren())
        {
            GameObject* child = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(uid);
            if (child && child->GetName().find("Arrow") != std::string::npos)
                child->SetEnabled(i == (size_t)selectedIndex);
        }
    }
}

void MainMenuSelectorScript::Update(float)
{
    if (!panelRoot) CachePanel_();
    if (!panelRoot) return;

    const bool panelEnabled = panelRoot->IsEnabled();

    // build on first open / first tick
    if (panelEnabled && !builtOnce)
    {
        BuildFromPanel_();
        selectedIndex = 0;
        UpdateSelection_();
        builtOnce = true;
    }

    if (!panelEnabled || menuItems.empty()) return;

    // self-correct: ensure exactly one arrow ON
    int arrowsOn = 0;
    for (auto* a : arrowImages)
        if (a && a->IsEnabled()) ++arrowsOn;
    if (arrowsOn != 1) UpdateSelection_();

    const KeyState* keys           = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* gamepadButtons = AppEngine->GetInputModule()->GetControllerButtons();
    const float2& leftStick        = AppEngine->GetInputModule()->GetLeftStick();

    bool moveDown                  = keys[SDL_SCANCODE_DOWN] == KEY_DOWN ||
                    gamepadButtons[SDL_CONTROLLER_BUTTON_DPAD_DOWN] == KEY_DOWN || (leftStick.y > 0.5f && !stickMoved);

    bool moveUp = keys[SDL_SCANCODE_UP] == KEY_DOWN || gamepadButtons[SDL_CONTROLLER_BUTTON_DPAD_UP] == KEY_DOWN ||
                  (leftStick.y < -0.5f && !stickMoved);

    if (moveDown)
    {
        selectedIndex = (selectedIndex + 1) % (int)menuItems.size();
        UpdateSelection_();
        stickMoved = true;
    }
    else if (moveUp)
    {
        selectedIndex = (selectedIndex - 1 + (int)menuItems.size()) % (int)menuItems.size();
        UpdateSelection_();
        stickMoved = true;
    }

    if (std::fabs(leftStick.y) < 0.3f) stickMoved = false;

    if (keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN ||
        gamepadButtons[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN)
    {
        GameObject* selectedItem = menuItems[selectedIndex];

        if (selectedItem->GetName() == "MenuItem_Continue")
        {
            if (gameOverCtrl) gameOverCtrl->Close();
            else if (pauseCtrl) pauseCtrl->Close();
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

    // if the panel gets disabled (scene change or close), allow rebuild next time
    if (!panelEnabled) builtOnce = false;
}
