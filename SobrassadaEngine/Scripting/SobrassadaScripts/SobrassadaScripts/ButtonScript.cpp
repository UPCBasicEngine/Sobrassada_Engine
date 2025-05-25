#include "pch.h"

#include "Application.h"
#include "ButtonScript.h"
#include "Component.h"
#include "Delegate.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "GameUIModule.h"
#include "Scene.h"
#include "SceneModule.h"
#include "Standalone/UI/ButtonComponent.h"
#include <imgui.h>
#include <string>

bool ButtonScript::Init()
{
    // GLOG("Initiating ButtonScript");

    if (!parent)
    {
        GLOG("ButtonScript::Init() failed: parent is null.");
        return false;
    }

    ButtonComponent* button = parent->GetComponent<ButtonComponent*>();

    if (button)
    {
        std::function<void(void)> function = std::bind(&ButtonScript::OnClick, this);
        Delegate<void> delegate(function);
        delegateID            = button->AddOnClickCallback(delegate);
        hasRegisteredCallback = true;
    }

    return true;
}

ButtonScript::~ButtonScript()
{
    if (hasRegisteredCallback)
    {

        ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
        if (button) button->RemoveOnClickCallback(delegateID);
    }
}

void ButtonScript::Update(float deltaTime)
{
}

void ButtonScript::Inspector()
{
    ImGui::SetCurrentContext(AppEngine->GetEditorUIModule()->GetImGuiContext());
    AppEngine->GetEditorUIModule()->DrawScriptInspector(
        [this]()
        {
            char buffer[128];
            strncpy_s(buffer, sizeof(buffer), panelToShowName.c_str(), _TRUNCATE);
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText("Panel to Show", buffer, sizeof(buffer)))
            {
                panelToShowName = buffer;
            }
        }
    );
}

void ButtonScript::OnClick()
{
    const std::string panelToHideName = GetCurrentPanelName();
    if (panelToHideName.empty())
    {
        return;
    }

    Scene* scene            = AppEngine->GetSceneModule()->GetScene();
    const auto& gameObjects = scene->GetAllGameObjects();

    for (const auto& [uid, go] : gameObjects)
    {
        if (go && go->GetName() == panelToHideName)
        {
            go->SetEnabledRecursive(false);
        }

        if (go && go->GetName() == panelToShowName)
        {

            go->SetEnabledRecursive(true);
        }
    }
}

std::string ButtonScript::GetCurrentPanelName() const
{
    GameObject* go = parent;
    while (go)
    {
        if (go->GetName().find("Panel") != std::string::npos)
        {
            return go->GetName();
        }
        go = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(go->GetParent());
    }
    return "";
}

// To save and Load the values input in PANELS
void ButtonScript::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator)
{
    targetState.AddMember("PanelToShow", rapidjson::Value(panelToShowName.c_str(), allocator), allocator);
}

void ButtonScript::Load(const rapidjson::Value& initialState)
{
    if (initialState.HasMember("PanelToShow") && initialState["PanelToShow"].IsString())
    {
        panelToShowName = initialState["PanelToShow"].GetString();
    }
}

void ButtonScript::OnDestroy()
{
    hasRegisteredCallback = false;
    delegateID            = {};
}