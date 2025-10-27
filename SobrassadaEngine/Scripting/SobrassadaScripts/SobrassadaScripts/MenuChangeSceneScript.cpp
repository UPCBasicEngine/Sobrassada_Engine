#include "pch.h"

#include "Application.h"
#include "Delegate.h"
#include "FileSystem/FileSystem.h"
#include "GameObject.h"
#include "GameSession.h"
#include "Globals.h"
#include "MenuChangeSceneScript.h"
#include "ProjectModule.h"
#include "SceneModule.h"
#include "Standalone/UI/ButtonComponent.h"

MenuChangeSceneScript::MenuChangeSceneScript(GameObject* parent) : Script(parent)
{
    fields.push_back({"Target Scene Name", InspectorField::FieldType::InputText, &targetSceneName});
}

MenuChangeSceneScript::~MenuChangeSceneScript() noexcept
{
    if (hasRegisteredCallback && parent)
    {
        ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
        if (button) button->RemoveOnClickCallback(delegateID);
    }
    parent = nullptr;
}

bool MenuChangeSceneScript::Init()
{
    scenesPath              = AppEngine->GetProjectModule()->GetLoadedProjectPath() + SCENES_PLAY_PATH;
    fullScenePath           = scenesPath + targetSceneName + SCENE_EXTENSION;

    ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
    if (button)
    {
        std::function<void(void)> function = std::bind(&MenuChangeSceneScript::OnClick, this);
        Delegate<void> delegate(function);
        delegateID            = button->AddOnClickCallback(delegate);
        hasRegisteredCallback = true;
    }

    return true;
}

void MenuChangeSceneScript::Update(float deltaTime)
{
}

void MenuChangeSceneScript::OnClick()
{
    if (sceneLoaded) return;

    sceneLoaded = true;
    if (targetSceneName == "SCENE_Tutorial") gNewGame = true;
    AppEngine->GetSceneModule()->RequestSceneLoad(fullScenePath);
}

void MenuChangeSceneScript::OnDestroy()
{
    hasRegisteredCallback = false;
    delegateID            = {};
}