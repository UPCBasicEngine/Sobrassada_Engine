#include "pch.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameOverScript.h"
#include "GameTimer.h"
#include "Globals.h"
#include "ProjectModule.h"
#include "SDL.h"
#include "Scene.h"
#include "SceneModule.h"
#include "ScriptComponent.h"
#include "Standalone/UI/ButtonComponent.h"

#include <algorithm>
#include <cstring>

bool gGameOverActive = false;

// -------- C entry point to request Game Over --------
extern "C" void GO_RequestGameOver()
{
    Scene* s = AppEngine->GetSceneModule()->GetScene();
    if (!s) return;

    if (GameObject* ctrl = s->GetGameObjectByName("GameOverController"))
        if (auto* sc = ctrl->GetComponent<ScriptComponent*>())
            if (auto* go = sc->GetScriptByType<GameOverScript>())
            {
                // GLOG: trigger requested through controller
                // GLOG("[GAMEOVER] GO_RequestGameOver -> instance=%p owner='%s'", go, ctrl->GetName().c_str());
                go->TriggerGameOver();
                return;
            }

    // fallback: search all objects
    const auto& all = s->GetAllGameObjects();
    for (const auto& kv : all)
    {
        GameObject* g = s->GetGameObjectByUID(kv.first);
        if (!g) continue;
        if (auto* sc = g->GetComponent<ScriptComponent*>())
            if (auto* go = sc->GetScriptByType<GameOverScript>())
            {
                // GLOG: trigger requested through fallback search
                // GLOG("[GAMEOVER] GO_RequestGameOver (fallback) -> instance=%p owner='%s'", go, g->GetName().c_str());
                go->TriggerGameOver();
                return;
            }
    }
}

GameOverScript::GameOverScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Panel To Show", InspectorField::FieldType::InputText, &panelToShowName});
}

bool GameOverScript::Init()
{
    gGameOverActive = false;

    CachePanel();
    if (panelRoot) panelRoot->SetEnabledRecursive(false);

    builtOnce     = false;
    gameOverShown = false;
    selectedIndex = 0;
    return true;
}

void GameOverScript::Inspector()
{
    ImGui::SetCurrentContext(AppEngine->GetEditorUIModule()->GetImGuiContext());
    AppEngine->GetEditorUIModule()->DrawScriptInspector(
        [this]()
        {
            char buffer[128];
            strncpy_s(buffer, sizeof(buffer), panelToShowName.c_str(), _TRUNCATE);
            buffer[sizeof(buffer) - 1] = '\0';
            if (ImGui::InputText("Panel To Show", buffer, sizeof(buffer)))
            {
                panelToShowName = buffer;
                panelRoot       = nullptr;
                builtOnce       = false;
            }
        }
    );
}

void GameOverScript::Update(float dt)
{
    if (!panelRoot) CachePanel();
    if (!gameOverShown || !panelRoot) return;

    if (!panelRoot->IsEnabled())
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
        // GLOG: menu constructed (first time build)
        // GLOG("[GAMEOVER] Menu built -> items=%zu", menuItems.size());
    }

    // ensure exactly one arrow is enabled
    int on = 0;
    for (auto* a : arrowImages)
        if (a && a->IsEnabled()) ++on;
    if (on != 1) UpdateSelection();
}

void GameOverScript::TriggerGameOver()
{
    if (gameOverShown) return;

    CachePanel();
    if (panelRoot)
    {
        panelRoot->SetEnabledRecursive(true);
        panelRoot->UpdateTransformForGOBranch();

        BuildFromPanel();
        selectedIndex = 0;
        DisableAllArrows();
        UpdateSelection();
        // GLOG: first paint with initial selection
        // GLOG("[GAMEOVER] First paint -> sel=%d (items=%zu)", selectedIndex, menuItems.size());
    }

    if (auto* t = AppEngine->GetGameTimer(); t && !t->IsPaused()) t->TogglePause();

    gGameOverActive = true;
    gameOverShown   = true;

    // GLOG: game over triggered, panel activated
    // GLOG("[GAMEOVER] TriggerGameOver -> panel '%s' activated", panelRoot ? panelRoot->GetName().c_str() : "(null)");
}

void GameOverScript::Close()
{
    if (panelRoot) panelRoot->SetEnabledRecursive(false);
    if (auto* timer = AppEngine->GetGameTimer(); timer && timer->IsPaused()) timer->TogglePause();

    gGameOverActive = false;
    gameOverShown   = false;
    builtOnce       = false;
}

void GameOverScript::CachePanel()
{
    Scene* scene = AppEngine->GetSceneModule()->GetScene();
    if (!scene) return;

    auto hasMenuChildren = [&](GameObject* go) -> bool
    {
        if (!go) return false;
        for (UID cid : go->GetChildren())
        {
            GameObject* ch = scene->GetGameObjectByUID(cid);
            if (ch && ch->GetName().find("MenuItem_") != std::string::npos) return true;
        }
        return false;
    };

    if (hasMenuChildren(parent)) panelRoot = parent;

    if (!panelRoot && !panelToShowName.empty())
        if (GameObject* byName = scene->GetGameObjectByName(panelToShowName)) panelRoot = byName;

    if (!panelRoot)
    {
        const auto& objects = scene->GetAllGameObjects();
        for (const auto& kv : objects)
        {
            GameObject* go = scene->GetGameObjectByUID(kv.first);
            if (!go) continue;
            if (hasMenuChildren(go))
            {
                panelRoot = go;
                break;
            }
        }
    }

    if (panelRoot)
    {
        // GLOG: cache panel found
        // GLOG("[GAMEOVER] CachePanel -> %s (target='%s')", panelRoot->GetName().c_str(), panelToShowName.c_str());
    }
    else
    {
        // GLOG: cache panel not found
        // GLOG("[GAMEOVER] CachePanel -> NOT FOUND (target='%s')", panelToShowName.c_str());
    }
}

void GameOverScript::BuildFromPanel()
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

    DisableAllArrows();
    // GLOG: menu items and arrows found
    // GLOG("[GAMEOVER] BuildFromPanel: items=%zu arrows=%zu", menuItems.size(), arrowImages.size());
}

void GameOverScript::DisableAllArrows()
{
    for (auto* a : arrowImages)
        if (a) a->SetEnabled(false);
}

void GameOverScript::UpdateSelection()
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
    // GLOG: selection changed
    // GLOG("[GAMEOVER] UpdateSelection -> sel=%d", selectedIndex);
}
