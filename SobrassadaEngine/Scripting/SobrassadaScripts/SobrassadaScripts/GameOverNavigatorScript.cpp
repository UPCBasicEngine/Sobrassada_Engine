#include "pch.h"

#include "GameOverNavigatorScript.h"

#include <algorithm> // std::clamp
#include <cmath>

#include "Application.h"
#include "GameObject.h"
#include "GameOverScript.h"
#include "InputModule.h"
#include "ProjectModule.h"
#include "SDL.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/UI/ButtonComponent.h"
#include "Wwise_IDs.h"

// Need the complete type for Respawn()
#include "CuChulainn.h"

// global player declared in CuChulainn.cpp
extern CuChulainn* playerScript;

bool GameOverNavigatorScript::Init()
{
    LocateGameOverScript();
    CachePanel();

    builtOnce     = false;
    selectedIndex = 0;

    lastMs        = GetCurrentTimeMs();
    repeatMs      = 0;
    lastDir       = 0;
    acceptWas     = false;
    upPrev = downPrev = accPrev = false;
    stickPrev                   = 0;

    audio                       = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] GameOverNavigatorScript: No audio component found");

    return true;
}

void GameOverNavigatorScript::Update(float)
{
    CachePanel();
    if (!panelRoot || !panelRoot->IsEnabled())
    {
        builtOnce = false;
        return;
    }

    if (!builtOnce)
    {
        BuildFromPanel();
        selectedIndex = 0;
        UpdateSelection();
        builtOnce = true;
        // GLOG: first time build completed
        // GLOG("[GONAV] Built -> items=%zu arrows=%zu", menuItems.size(), arrowImages.size());
    }

    // ensure exactly one arrow is enabled
    int on = 0;
    for (auto* a : arrowImages)
        if (a && a->IsEnabled()) ++on;
    if (on != 1) UpdateSelection();

    // Input
    bool upHeld = false, downHeld = false, accHeld = false;
    int stickDir = 0;
    ReadInputs(upHeld, downHeld, accHeld, stickDir);

    const bool upEdge        = upHeld && !upPrev;
    const bool downEdge      = downHeld && !downPrev;
    const bool accEdge       = accHeld && !accPrev;
    const bool stickUpEdge   = (stickDir == -1 && stickPrev == 0);
    const bool stickDownEdge = (stickDir == +1 && stickPrev == 0);

    int dir                  = 0;
    if (downEdge || stickDownEdge) dir = +1;
    else if (upEdge || stickUpEdge) dir = -1;
    else
    {
        int heldDir        = downHeld ? +1 : (upHeld ? -1 : 0);
        const uint64_t now = GetCurrentTimeMs();
        if (heldDir != 0)
        {
            if (lastDir != heldDir)
            {
                lastDir  = heldDir;
                repeatMs = 230; // initial delay
                lastMs   = now;
            }
            else
            {
                const uint32_t el = (uint32_t)(now - lastMs);
                if (el >= repeatMs)
                {
                    dir      = heldDir;
                    repeatMs = 140; // repeat interval
                    lastMs   = now;
                }
            }
        }
        else
        {
            lastDir  = 0;
            repeatMs = 0;
        }
    }

    if (dir != 0 && !menuItems.empty())
    {
        const int n   = (int)menuItems.size();
        selectedIndex = (selectedIndex + (dir > 0 ? +1 : -1) + n) % n;
        UpdateSelection();
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_02);
    }

    if (accEdge && !menuItems.empty())
    {
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_01);
        GameObject* item       = menuItems[selectedIndex];
        const std::string name = item ? item->GetName() : "(null)";
        // GLOG: accept pressed on current item
        // GLOG("[GONAV] Accept on '%s'", name.c_str());

        if (name == "MenuItem_Continue")
        {
            if (goController) goController->Close();
            if (playerScript) playerScript->Respawn(); // immediate respawn
            builtOnce = false;
            return;
        }
        else if (name == "MenuItem_Menu")
        {
            if (goController) goController->Close();
            AppEngine->GetSceneModule()->GetScene()->SetStopPlaying(true);
            std::string path =
                AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PATH + "SCENE_MainMenu.scene";
            AppEngine->GetSceneModule()->RequestSceneLoad(path);
            builtOnce = false;
            return;
        }
        else
        {
            if (auto* btn = item->GetComponent<ButtonComponent*>()) btn->OnClick();
        }
    }

    // save previous states
    upPrev    = upHeld;
    downPrev  = downHeld;
    accPrev   = accHeld;
    stickPrev = stickDir;
    acceptWas = accHeld;
}

// ================= Helpers =================

void GameOverNavigatorScript::CachePanel()
{
    if (panelRoot && panelRoot->GetName() == panelName) return;

    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    if (!scene)
    {
        panelRoot = nullptr;
        return;
    }

    if (GameObject* byName = scene->GetGameObjectByName(panelName))
    {
        panelRoot = byName;
        return;
    }

    // fallback: first GO with children named "MenuItem_"
    const auto& objects = scene->GetAllGameObjects();
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
            return;
        }
    }
}

void GameOverNavigatorScript::BuildFromPanel()
{
    if (!panelRoot) return;

    menuItems.clear();
    arrowImages.clear();

    Scene* s = AppEngine->GetSceneModule()->GetScene();
    for (UID cid : panelRoot->GetChildren())
    {
        GameObject* child = s->GetGameObjectByUID(cid);
        if (!child) continue;

        if (child->GetName().find("MenuItem_") != std::string::npos)
        {
            menuItems.push_back(child);
            for (UID gcid : child->GetChildren())
            {
                GameObject* arrow = s->GetGameObjectByUID(gcid);
                if (arrow && arrow->GetName().find("Arrow") != std::string::npos) arrowImages.push_back(arrow);
            }
        }
    }

    for (auto* a : arrowImages)
        if (a) a->SetEnabled(false);

    selectedIndex = std::clamp(selectedIndex, 0, (int)menuItems.size() - 1);
    UpdateSelection();

    // GLOG: items discovered for navigation
    // GLOG("[GONAV] BuildFromPanel: items=%zu arrows=%zu", menuItems.size(), arrowImages.size());
}

void GameOverNavigatorScript::UpdateSelection()
{
    if (menuItems.empty()) return;
    selectedIndex = std::clamp(selectedIndex, 0, (int)menuItems.size() - 1);

    Scene* s      = AppEngine->GetSceneModule()->GetScene();
    for (size_t i = 0; i < menuItems.size(); ++i)
    {
        for (UID uid : menuItems[i]->GetChildren())
        {
            GameObject* child = s->GetGameObjectByUID(uid);
            if (child && child->GetName().find("Arrow") != std::string::npos)
                child->SetEnabled(i == (size_t)selectedIndex);
        }
    }
    // GLOG: selection updated
    // GLOG("[GONAV] UpdateSelection -> sel=%d", selectedIndex);
}

void GameOverNavigatorScript::LocateGameOverScript()
{
    Scene* s = AppEngine->GetSceneModule()->GetScene();
    if (!s) return;

    if (GameObject* ctrl = s->GetGameObjectByName("GameOverController"))
        if (auto* sc = ctrl->GetComponent<ScriptComponent*>())
            if (auto* go = sc->GetScriptByType<GameOverScript>())
            {
                goController = go;
                return;
            }

    const auto& all = s->GetAllGameObjects();
    for (const auto& kv : all)
    {
        GameObject* g = s->GetGameObjectByUID(kv.first);
        if (!g) continue;
        if (auto* sc = g->GetComponent<ScriptComponent*>())
            if (auto* go = sc->GetScriptByType<GameOverScript>())
            {
                goController = go;
                return;
            }
    }
}

uint64_t GameOverNavigatorScript::GetCurrentTimeMs() const
{
    const uint64_t c = (uint64_t)SDL_GetPerformanceCounter();
    const uint64_t f = (uint64_t)SDL_GetPerformanceFrequency();
    return (f > 0) ? (c * 1000ull) / f : 0ull;
}

void GameOverNavigatorScript::ReadInputs(bool& up, bool& down, bool& acc, int& stickDir)
{
    SDL_PumpEvents();
    SDL_GameControllerUpdate();

    // keyboard state
    const Uint8* kb          = SDL_GetKeyboardState(nullptr);
    const bool kUp           = (kb[SDL_SCANCODE_UP] != 0) || (kb[SDL_SCANCODE_W] != 0);
    const bool kDown         = (kb[SDL_SCANCODE_DOWN] != 0) || (kb[SDL_SCANCODE_S] != 0);
    const bool kEnt          = (kb[SDL_SCANCODE_RETURN] != 0) || (kb[SDL_SCANCODE_KP_ENTER] != 0);
    const bool kSpc          = (kb[SDL_SCANCODE_SPACE] != 0);

    // gamepad via InputModule
    const InputModule* input = AppEngine->GetInputModule();
    const KeyState* buttons  = input ? input->GetControllerButtons() : nullptr;
    const bool padUpHeld     = buttons ? (buttons[SDL_CONTROLLER_BUTTON_DPAD_UP] != KEY_IDLE) : false;
    const bool padDownHeld   = buttons ? (buttons[SDL_CONTROLLER_BUTTON_DPAD_DOWN] != KEY_IDLE) : false;
    const bool padAHeld      = buttons ? (buttons[SDL_CONTROLLER_BUTTON_A] != KEY_IDLE) : false;
    const float2& ls         = input ? input->GetLeftStick() : float2 {0.f, 0.f};

    up                       = kUp || padUpHeld || (ls.y < -0.5f);
    down                     = kDown || padDownHeld || (ls.y > 0.5f);
    acc                      = kEnt || kSpc || padAHeld;

    stickDir                 = (ls.y > 0.5f) ? +1 : ((ls.y < -0.5f) ? -1 : 0);
}
