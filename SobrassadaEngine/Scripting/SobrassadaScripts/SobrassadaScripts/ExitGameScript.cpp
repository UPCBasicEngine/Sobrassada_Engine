#include "pch.h"

#include "Application.h"
#include "Delegate.h"
#include "EditorUIModule.h"
#include "ExitGameScript.h"
#include "GameObject.h"
#include "Standalone/UI/ButtonComponent.h"

bool ExitGameScript::Init()
{
    ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
    if (button)
    {
        std::function<void(void)> function = std::bind(&ExitGameScript::OnClick, this);
        Delegate<void> delegate(function);
        delegateID            = button->AddOnClickCallback(delegate);
        hasRegisteredCallback = true;
    }

    return true;
}

void ExitGameScript::Update(float deltaTime)
{
}

void ExitGameScript::Inspector()
{
}

void ExitGameScript::OnClick()
{
    //GLOG("Exiting game...");
    AppEngine->GetEditorUIModule()->RequestExit();
}

ExitGameScript::~ExitGameScript()
{
    if (hasRegisteredCallback)
    {
        ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
        if (button) button->RemoveOnClickCallback(delegateID);
    }
}

void ExitGameScript::OnDestroy()
{
    //GLOG("ExitGameScript::OnDestroy() called");

    hasRegisteredCallback = false;
    delegateID            = {};
}