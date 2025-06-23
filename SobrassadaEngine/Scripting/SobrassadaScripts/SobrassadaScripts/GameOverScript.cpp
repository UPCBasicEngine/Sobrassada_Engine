#include "pch.h"

#include "GameOverScript.h"
#include "Application.h"
#include "CuChulainn.h"
#include "GameObject.h"
#include "GameTimer.h"
#include "Scene.h"
#include "EditorUIModule.h"
#include "SceneModule.h"

bool GameOverScript::Init()
{
    CachePanel();
    if (cachedTarget)
        cachedTarget->SetEnabledRecursive(false);
    return true;
}

void GameOverScript::Update(float deltaTime)
{
    if (gameOverShown) return;

    if (!cachedTarget)
        CachePanel();

    if (playerScript && playerScript->IsDead()) TriggerGameOver();
}

void GameOverScript::TriggerGameOver()
{
    if (gameOverShown) return;

    ShowPanel();
    PauseGame();
    gameOverShown = true;
}

void GameOverScript::CachePanel()
{
    if (cachedTarget) return;

    const auto& allGOs = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();

    for (const auto& [uid, go] : allGOs)
        if (go && go->GetName() == panelToShowName)
        {
            cachedTarget = go;
            break;
        }
}

void GameOverScript::ShowPanel()
{
    CachePanel();
    if (cachedTarget) cachedTarget->SetEnabledRecursive(true);
}

void GameOverScript::PauseGame()
{
    if (auto* timer = AppEngine->GetGameTimer()) timer->TogglePause();
}


void GameOverScript::Inspector()
{
    AppEngine->GetEditorUIModule()->DrawScriptInspector(fields);
}

void GameOverScript::Save(rapidjson::Value& state, rapidjson::Document::AllocatorType& alloc)
{
    state.AddMember("PanelToShow", rapidjson::Value(panelToShowName.c_str(), alloc), alloc);
}

void GameOverScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("PanelToShow") && initialState["PanelToShow"].IsString())
    {
        panelToShowName = initialState["PanelToShow"].GetString();
    }
}
