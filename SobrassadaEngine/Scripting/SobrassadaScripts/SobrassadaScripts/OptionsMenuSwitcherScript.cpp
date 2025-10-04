#include "pch.h"

#include "Application.h"
#include "Components/Standalone/UI/ImageComponent.h"
#include "GameObject.h"
#include "InputModule.h"
#include "OptionsMenuSwitcherScript.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/Audio/AudioSourceComponent.h"
#include "Wwise_IDs.h"

const std::unordered_map<std::string, TexPair> OptionsMenuSwitcherScript::panelInput = {
    {"OptionsKeyboardPanel",   {1203489876831052, 1295999750777550}},
    {"OptionsControllerPanel", {1202209373146889, 1270492191063579}},
    {"OptionsAudioPanel",      {1207353832276846, 1250092281844907}},
    {"OptionsVideoPanel",      {1204790345600293, 1206017489546089}}
};

const std::vector<std::string> OptionsMenuSwitcherScript::panelNames = {
    "OptionsKeyboardPanel", "OptionsControllerPanel", "OptionsAudioPanel", "OptionsVideoPanel"
};

bool OptionsMenuSwitcherScript::Init()
{
    lastKbState = AppEngine->GetInputModule()->IsUsingKeyboard();
    ApplyDeviceTextures(lastKbState);
    ShowOnlyCurrentPanel();
    audio = parent->GetComponent<AudioSourceComponent*>();
    if (!audio) GLOG("[WARNING] OptionsMenuSwitcherScript: No audio component found");
    return true;
}

void OptionsMenuSwitcherScript::Update(float deltaTime)
{
    if (!parent->IsEnabled()) return;

    if (!initialized)
    {
        ShowOnlyCurrentPanel();
        initialized = true;
    }

    bool nowKb = AppEngine->GetInputModule()->IsUsingKeyboard();
    if (nowKb != lastKbState)
    {
        ApplyDeviceTextures(nowKb);
        lastKbState = nowKb;
    }

    const KeyState* keys           = AppEngine->GetInputModule()->GetKeyboard();
    const KeyState* gamepadButtons = AppEngine->GetInputModule()->GetControllerButtons();

    if (keys[SDL_SCANCODE_Q] == KEY_DOWN || keys[SDL_SCANCODE_E] == KEY_DOWN ||
        gamepadButtons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] == KEY_DOWN ||
        gamepadButtons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] == KEY_DOWN)
    {
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_02);

        // Deactivate current
        GameObject* currentGO = FindPanelByName(panelNames[currentIndex]);
        if (currentGO) currentGO->SetEnabled(false);

        // Update index
        if (keys[SDL_SCANCODE_Q] == KEY_DOWN || gamepadButtons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] == KEY_DOWN)
            currentIndex = (currentIndex - 1 + panelNames.size()) % static_cast<int>(panelNames.size());
        else if (keys[SDL_SCANCODE_E] == KEY_DOWN || gamepadButtons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] == KEY_DOWN)
            currentIndex = (currentIndex + 1) % panelNames.size();

        // Activate new
        ShowOnlyCurrentPanel();
    }

    if (keys[SDL_SCANCODE_ESCAPE] == KEY_DOWN || gamepadButtons[SDL_CONTROLLER_BUTTON_B] == KEY_DOWN)
    {
        if (audio) audio->EmitEvent(AK::EVENTS::PLAY_SFX_BUTTON_03);

        // Disable all panels in the options menu
        for (const std::string& name : panelNames)
        {
            GameObject* panel = FindPanelByName(name);
            if (panel) panel->SetEnabled(false);
        }

        // Enable the MainMenuPanel
        GameObject* mainMenuPanel = FindPanelByName("MainMenuPanel");
        if (mainMenuPanel) mainMenuPanel->SetEnabledRecursive(true);

        parent->SetEnabledRecursive(false);
        initialized = false;
    }
}
void OptionsMenuSwitcherScript::ApplyDeviceTextures(bool usingKb)
{
    for (const std::string& name : panelNames)
    {
        GameObject* go = FindPanelByName(name);
        if (!go) continue;

        ImageComponent* img = go->GetComponent<ImageComponent*>();
        if (!img) continue;

        const TexPair& tp = panelInput.at(name);
        img->ChangeTexture(usingKb ? tp.texKeyboard : tp.texGamepad);
    }
}

void OptionsMenuSwitcherScript::ShowOnlyCurrentPanel()
{
    for (int i = 0; i < panelNames.size(); ++i)
    {
        GameObject* panel = FindPanelByName(panelNames[i]);
        if (panel) panel->SetEnabled(i == currentIndex);
    }
}

GameObject* OptionsMenuSwitcherScript::FindPanelByName(const std::string& name) const
{
    const auto& gameObjects = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();
    for (const auto& pair : gameObjects)
    {
        if (pair.second->GetName() == name) return pair.second;
    }
    return nullptr;
}

void OptionsMenuSwitcherScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("CurrentPanelIndex", currentIndex, allocator);
}

void OptionsMenuSwitcherScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("CurrentPanelIndex") && initialState["CurrentPanelIndex"].IsInt())
    {
        currentIndex = initialState["CurrentPanelIndex"].GetInt();
    }
}
