#include "GameOverScript.h"
#include "GameUIModule.h"
#include "InputModule.h"
#include "SceneModule.h"

bool GameOverScript::Init()
{
    if (canvasGO) canvasGO->SetEnabled(false);
    return true;
}

void GameOverScript::OnPlayerDeath()
{
    pending = true;
    timer   = 0.0f;
}

void GameOverScript::Update(float dt)
{
    if (!pending) return;

    timer += dt;
    if (timer >= showDelay)
    {
        pending = false;
        if (canvasGO) canvasGO->SetEnabled(true);

        AppEngine->GetInputModule()->SetEnabled(false);
        AppEngine->GetSceneModule()->SetTimeScale(0);
    }

}

void GameOverScript::Inspector()
{
    fields = {
        {"Delay (s)", InspectorField::FieldType::Float, &showDelay, 0.f, 10.f},
        {"GameOver Canvas", InspectorField::FieldType::GameObject, &canvasGO}
    };
    Script::Inspector();
}

void GameOverScript::Save(rapidjson::Value& val, rapidjson::Document::AllocatorType& al)
{
    Script::Save(val, al);
    val.AddMember("Delay", showDelay, al);
    UID uid = canvasGO ? canvasGO->GetUID() : 0;
    val.AddMember("CanvasUID", uid, al);
}

void GameOverScript::Load(const rapidjson::Value& val)
{
    Script::Load(val);
    if (val.HasMember("Delay")) showDelay = val["Delay"].GetFloat();
    if (val.HasMember("CanvasUID"))
        canvasGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(val["CanvasUID"].GetUint64());
}
