#include "pch.h"

#include "Application.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Globals.h"
#include "InputModule.h"
#include "PauseMenuScript.h"

#include "MusicManager.h"
#include "ProjectModule.h"
#include "SDL.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Standalone/UI/ButtonComponent.h"
#include "Wwise_IDs.h"
#include <algorithm>

class MusicManager;
extern bool gGameOverActive; // block pause if Game Over is active

bool PauseMenuScript::Init()
{
    CachePanel();
    if (cachedTarget) cachedTarget->SetEnabledRecursive(false);

    isOpen        = false;
    builtOnce     = false;
    selectedIndex = 0;

    lastMs        = GetCurrentTimeMs();
    repeatMs      = 0;
    lastDir       = 0;
    acceptWas     = false;
    upPrev = downPrev = accPrev = false;

    audio                       = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] PauseMenuScript: No audio component found");

    return true;
}

void PauseMenuScript::Show()
{
    if (gGameOverActive) return; // do not open pause on Game Over
    if (isOpen) return;
    CachePanel();
    if (cachedTarget)
    {
        cachedTarget->SetEnabledRecursive(true);
        cachedTarget->UpdateTransformForGOBranch();
        // initial render/setup
        BuildFromPanel();
        selectedIndex = 0;
        DisableAllArrows();
        UpdateSelection();
    }

    if (auto* t = AppEngine->GetGameTimer(); t && !t->IsPaused()) t->TogglePause();
    isOpen    = true;
    builtOnce = true;

    if (audio != nullptr)
    {
        GLOG("Switching Gamestate to menu")
        audio->EmitEvent(AK::EVENTS::SET_GAMESTATE_MENU);
    }

    // GLOG("[PAUSE] Show -> panel='%s'", cachedTarget ? cachedTarget->GetName().c_str() : "(null)");
}

void PauseMenuScript::Close()
{
    if (!isOpen) return;

    if (cachedTarget) cachedTarget->SetEnabledRecursive(false);
    if (auto* t = AppEngine->GetGameTimer(); t && t->IsPaused()) t->TogglePause();

    isOpen    = false;
    builtOnce = false;

    GameObject* musicManager = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByName("MusicManager");
    if (musicManager != nullptr)
        musicManager->GetComponent<ScriptComponent*>()->GetScriptByType<MusicManager>()->ResetToCachedGameState();

    // GLOG("[PAUSE] Close");
}

void PauseMenuScript::Toggle()
{
    isOpen ? Close() : Show();
}

void PauseMenuScript::Update(float)
{
    if (gGameOverActive) return;

    const InputModule* in = AppEngine->GetInputModule();
    const KeyState* pad   = in ? in->GetControllerButtons() : nullptr;

    bool padToggleHeld    = false;
    if (pad)
    {
        const bool startHeld = (pad[SDL_CONTROLLER_BUTTON_START] != KEY_IDLE);
        const bool backHeld  = (pad[SDL_CONTROLLER_BUTTON_BACK] != KEY_IDLE);
        const bool bHeld     = (isOpen && (pad[SDL_CONTROLLER_BUTTON_B] != KEY_IDLE));
        padToggleHeld        = startHeld || backHeld || bHeld;
    }

    static bool padTogglePrev = false;
    const bool padToggleEdge  = padToggleHeld && !padTogglePrev;
    padTogglePrev             = padToggleHeld;

    const KeyState* k         = AppEngine->GetInputModule()->GetKeyboard();
    if (padToggleEdge || (k[SDL_SCANCODE_ESCAPE] == KEY_DOWN))
    {
        Toggle();
        return;
    }

    if (isOpen) HandleInput();
}

void PauseMenuScript::Save(rapidjson::Value& out, rapidjson::Document::AllocatorType& a)
{
    out.AddMember("PanelToShow", rapidjson::Value(panelToShowName.c_str(), a), a);
}

void PauseMenuScript::Load(const rapidjson::Value& in)
{
    if (in.HasMember("PanelToShow") && in["PanelToShow"].IsString()) panelToShowName = in["PanelToShow"].GetString();
}

void PauseMenuScript::CachePanel()
{
    if (cachedTarget != nullptr) return;

    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    if (!scene) return;

    // exact name
    if (GameObject* go = scene->GetGameObjectByName(panelToShowName))
    {
        cachedTarget = go;
        // GLOG("[PAUSE] CachePanel -> %s", cachedTarget->GetName().c_str());
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
            cachedTarget = go;
            break;
        }
    }
    // GLOG("[PAUSE] CachePanel -> %s", cachedTarget ? cachedTarget->GetName().c_str() : "NOT FOUND");
}

void PauseMenuScript::BuildFromPanel()
{
    if (!cachedTarget) return;

    menuItems.clear();
    arrowImages.clear();

    Scene* s = AppEngine->GetSceneModule()->GetScene();
    for (UID cid : cachedTarget->GetChildren())
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

    DisableAllArrows();
    // GLOG("[PAUSE] BuildFromPanel: items=%zu arrows=%zu", menuItems.size(), arrowImages.size());
}

void PauseMenuScript::DisableAllArrows()
{
    for (auto* a : arrowImages)
        if (a) a->SetEnabled(false);
}

void PauseMenuScript::UpdateSelection()
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
    // GLOG("[PAUSE] UpdateSelection -> sel=%d", selectedIndex);
}

uint64_t PauseMenuScript::GetCurrentTimeMs() const
{
    const uint64_t c = (uint64_t)SDL_GetPerformanceCounter();
    const uint64_t f = (uint64_t)SDL_GetPerformanceFrequency();
    return (f > 0) ? (c * 1000ull) / f : 0ull;
}

void PauseMenuScript::ReadInputs(bool& up, bool& down, bool& acc, int& stickDir)
{
    // keyboard
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

void PauseMenuScript::HandleInput()
{
    bool upHeld = false, downHeld = false, accHeld = false;
    int stickDir = 0;
    ReadInputs(upHeld, downHeld, accHeld, stickDir);

    const bool upEdge   = upHeld && !upPrev;
    const bool downEdge = downHeld && !downPrev;
    const bool accEdge  = accHeld && !accPrev;

    // direction handling (edge + autorepeat)
    int dir             = 0;
    if (downEdge) dir = +1;
    else if (upEdge) dir = -1;
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
        // GLOG("[PAUSE] Accept on '%s'", name.c_str());

        if (name == "MenuItem_Continue")
        {
            Close();
            return;
        }
        else if (name == "MenuItem_Menu")
        {
            Close();
            AppEngine->GetSceneModule()->GetScene()->SetStopPlaying(true);
            std::string path =
                AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH + "SCENE_MainMenu.scene";
            AppEngine->GetSceneModule()->RequestSceneLoad(path);
            return;
        }
        else
        {
            if (auto* btn = item->GetComponent<ButtonComponent*>()) btn->OnClick();
        }
    }

    // save previous states
    upPrev   = upHeld;
    downPrev = downHeld;
    accPrev  = accHeld;
}
