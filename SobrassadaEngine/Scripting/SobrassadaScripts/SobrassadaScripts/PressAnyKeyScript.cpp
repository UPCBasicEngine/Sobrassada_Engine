#include "pch.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "InputModule.h"
#include "PressAnyKeyScript.h"
#include "Scene.h"
#include "SceneModule.h"
#include <imgui.h>

bool PressAnyKeyScript::Init()
{
    if (!parent)
    {
        return false;
    }

    return true;
}

void PressAnyKeyScript::Update(float deltaTime)
{
    if (!parent || !parent->IsEnabled()) return;

    const KeyState* keys = AppEngine->GetInputModule()->GetKeyboard();

    if (keys[SDL_SCANCODE_RETURN] == KEY_DOWN || keys[SDL_SCANCODE_SPACE] == KEY_DOWN)
    {
        //GLOG("Valid key pressed - Hiding '{}', showing '{}'", parent->GetName(), nextGameObjectName);

        parent->SetEnabled(false);

        const auto& gameObjects = AppEngine->GetSceneModule()->GetScene()->GetAllGameObjects();
        for (const auto& [uid, go] : gameObjects)
        {
            if (go && go->GetName() == nextGameObjectName)
            {
                go->SetEnabled(true);
                //GLOG("Enabled GameObject '{}'", nextGameObjectName);
                break;
            }
        }
    }
}

void PressAnyKeyScript::Inspector()
{
    ImGui::SetCurrentContext(AppEngine->GetEditorUIModule()->GetImGuiContext());

    AppEngine->GetEditorUIModule()->DrawScriptInspector(
        [this]()
        {
            char buffer[128];
            strncpy_s(buffer, sizeof(buffer), nextGameObjectName.c_str(), _TRUNCATE);
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText("Next GameObject to Show", buffer, sizeof(buffer)))
            {
                nextGameObjectName = buffer;
            }
        }
    );
}

void PressAnyKeyScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("NextGameObject", rapidjson::Value(nextGameObjectName.c_str(), allocator), allocator);
}

void PressAnyKeyScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("NextGameObject") && initialState["NextGameObject"].IsString())
    {
        nextGameObjectName = initialState["NextGameObject"].GetString();
    }
}
