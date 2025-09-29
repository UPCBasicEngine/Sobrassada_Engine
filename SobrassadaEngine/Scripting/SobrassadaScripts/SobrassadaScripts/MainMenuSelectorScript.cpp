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
#include "Wwise_IDs.h"
#include <algorithm>
#include <cmath>

bool MainMenuSelectorScript::Init()
{
    CachePanel();

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

    // ensure initial state
    stickMoved = false;

    if (panelRoot && panelRoot->IsEnabled())
    {
        BuildFromPanel();
        UpdateSelection();
        builtOnce = true;
    }

    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] MainMenuSelectorScript: No audio component found");

    return true;
}

void MainMenuSelectorScript::CachePanel()
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

    // 2) Fallback: first GO that has children named "MenuItem_*"
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

    // GLOG: cache result
    // GLOG("[SEL] CachePanel -> %s (name=%s)", panelRoot ? "FOUND" : "NOT FOUND", panelName.c_str());
}

void MainMenuSelectorScript::BuildFromPanel()
{
    if (!panelRoot) return;

    menuItems.clear();
    arrowImages.clear();

    Scene* scene = AppEngine->GetSceneModule()->GetScene();

    for (UID cid : panelRoot->GetChildren())
    {
        GameObject* child = scene->GetGameObjectByUID(cid);
        if (!child) continue;

        if (child->GetName().find("MenuItem_") != std::string::npos)
        {
            menuItems.push_back(child);

            for (UID gcid : child->GetChildren())
            {
                GameObject* arrow = scene->GetGameObjectByUID(gcid);
                if (arrow && arrow->GetName().find("Arrow") != std::string::npos) arrowImages.push_back(arrow);
            }
        }
    }

    // turn all arrows off; UpdateSelection() will light the selected one
    for (auto* a : arrowImages)
        if (a) a->SetEnabled(false);

    // GLOG: discovered items/arrows
    // GLOG("[SEL] BuildFromPanel: items=%zu arrows=%zu", menuItems.size(), arrowImages.size());
}

void MainMenuSelectorScript::UpdateSelection()
{
    if (menuItems.empty()) return;

    selectedIndex = std::clamp(selectedIndex, 0, (int)menuItems.size() - 1);

    Scene* scene  = AppEngine->GetSceneModule()->GetScene();
    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        for (UID uid : menuItems[i]->GetChildren())
        {
            GameObject* child = scene->GetGameObjectByUID(uid);
            if (child && child->GetName().find("Arrow") != std::string::npos)
                child->SetEnabled(i == (size_t)selectedIndex);
        }
    }
    // GLOG: selection updated
    // GLOG("[SEL] UpdateSelection -> sel=%d", selectedIndex);
}

void MainMenuSelectorScript::Update(float)
{
    if (!panelRoot) CachePanel();
    if (!panelRoot) return;

    const bool panelEnabled = panelRoot->IsEnabled();

    // if panel just turned off, mark for rebuild and bail
    if (!panelEnabled)
    {
        builtOnce = false;
        return;
    }

    // build on first open / first tick
    if (!builtOnce)
    {
        BuildFromPanel();
        selectedIndex = 0;
        UpdateSelection();
        builtOnce = true;
    }

    if (menuItems.empty()) return;

    // self-correct: ensure exactly one arrow ON
    int arrowsOn = 0;
    for (auto* a : arrowImages)
        if (a && a->IsEnabled()) ++arrowsOn;
    if (arrowsOn != 1) UpdateSelection();

    const InputModule* input   = AppEngine->GetInputModule();
    const KeyState* keys       = input->GetKeyboard();
    const KeyState* padButtons = input->GetControllerButtons();
    const float2& leftStick    = input->GetLeftStick();

    const bool moveDown        = keys[SDL_SCANCODE_DOWN] == KEY_DOWN ||
                          padButtons[SDL_CONTROLLER_BUTTON_DPAD_DOWN] == KEY_DOWN ||
                          (leftStick.y > 0.5f && !stickMoved);

    const bool moveUp = keys[SDL_SCANCODE_UP] == KEY_DOWN || padButtons[SDL_CONTROLLER_BUTTON_DPAD_UP] == KEY_DOWN ||
                        (leftStick.y < -0.5f && !stickMoved);

    if (moveDown)
    {
        selectedIndex = (selectedIndex + 1) % (int)menuItems.size();
        UpdateSelection();
        stickMoved = true;
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_02);
    }
    else if (moveUp)
    {
        selectedIndex = (selectedIndex - 1 + (int)menuItems.size()) % (int)menuItems.size();
        UpdateSelection();
        stickMoved = true;
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_02);
    }

    if (std::fabs(leftStick.y) < 0.3f) stickMoved = false;

    const bool accept = keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN ||
                        padButtons[SDL_CONTROLLER_BUTTON_A] == KEY_DOWN;

    if (accept)
    {
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_01);
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
            if (auto* button = selectedItem->GetComponent<ButtonComponent*>()) button->OnClick();
        }
    }
}
