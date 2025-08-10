#include "pch.h"

#include "Application.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "InputModule.h"
#include "PauseMenuScript.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"

bool PauseMenuScript::Init()
{
    CachePanel();
    if (cachedTarget) cachedTarget->SetEnabledRecursive(false);
    isOpen = false;
    return true;
}

void PauseMenuScript::Show()
{
    if (isOpen) return;
    CachePanel();
    if (cachedTarget)
    {
        cachedTarget->SetEnabledRecursive(true);
        cachedTarget->UpdateTransformForGOBranch();
    }
    if (auto* t = AppEngine->GetGameTimer(); t && !t->IsPaused()) t->TogglePause();
    isOpen = true;
}

void PauseMenuScript::Close()
{
    if (!isOpen) return;
    if (cachedTarget) cachedTarget->SetEnabledRecursive(false);
    if (auto* t = AppEngine->GetGameTimer(); t && t->IsPaused()) t->TogglePause();
    isOpen = false;
}

void PauseMenuScript::Toggle()
{
    isOpen ? Close() : Show();
}

void PauseMenuScript::Update(float)
{
    const KeyState* k = AppEngine->GetInputModule()->GetKeyboard();
    if (k[SDL_SCANCODE_ESCAPE] == KEY_DOWN) Toggle();
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

    Scene* scene        = AppEngine->GetSceneModule()->GetScene();
    const auto& objects = scene->GetAllGameObjects();

    for (const auto& [uid, go] : objects)
    {
        if (go == nullptr)
            continue;

        if (go->GetName() == panelToShowName)
        {
            cachedTarget = go; 
            return;            
        }
    }
}
