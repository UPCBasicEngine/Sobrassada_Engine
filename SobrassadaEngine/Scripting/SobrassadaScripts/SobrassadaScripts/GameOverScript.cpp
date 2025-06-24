#include "pch.h"

#include "Application.h"
#include "CuChulainn.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameOverScript.h"
#include "GameTimer.h"
#include "Scene.h"
#include "SceneModule.h"

bool GameOverScript::Init()
{
    CachePanel();
    if (cachedTarget) cachedTarget->SetEnabledRecursive(false);
    return true;
}

void GameOverScript::Update(float)
{
    if (gameOverShown && cachedTarget && !cachedTarget->IsEnabled()) gameOverShown = false;

    if (gameOverShown) return;

    if (!cachedTarget) CachePanel();
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
    const auto& gos = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();
    for (const auto& [uid, go] : gos)
        if (go && go->GetName() == panelToShowName)
        {
            cachedTarget = go;
            break;
        }
}

void GameOverScript::ShowPanel()
{
    CachePanel();
    if (!cachedTarget) return;
    cachedTarget->UpdateMobilityHierarchy(DYNAMIC);
    cachedTarget->SetEnabledRecursive(true); 
    cachedTarget->UpdateTransformForGOBranch(); 
    cachedTarget->InitHierarchy();              
}


void GameOverScript::PauseGame()
{
    if (auto* t = AppEngine->GetGameTimer()) t->TogglePause();
}

void GameOverScript::Inspector()
{
    AppEngine->GetEditorUIModule()->DrawScriptInspector(fields);
}

void GameOverScript::Save(rapidjson::Value& st, rapidjson::Document::AllocatorType& a)
{
    st.AddMember("PanelToShow", rapidjson::Value(panelToShowName.c_str(), a), a);
}

void GameOverScript::Load(const rapidjson::Value& st)
{
    if (st.HasMember("PanelToShow") && st["PanelToShow"].IsString()) panelToShowName = st["PanelToShow"].GetString();
}
