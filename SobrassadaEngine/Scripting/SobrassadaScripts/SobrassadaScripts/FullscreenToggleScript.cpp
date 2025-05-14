#include "pch.h"

#include "Application.h"
#include "EditorUIModule.h"
#include "FullscreenToggleScript.h"
#include "GameObject.h"
#include "Standalone/UI/ButtonComponent.h"

bool FullscreenToggleScript::Init()
{
    ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
    if (button)
    {
        FullscreenToggleScript* self   = this;

        std::function<void()> function = [self]()
        {
            if (self) self->OnClick();
        };

        Delegate<void> delegate(function);
        delegateID            = button->AddOnClickCallback(delegate);
        hasRegisteredCallback = true;
    }

    return true;
}

void FullscreenToggleScript::Update(float deltaTime)
{
}

void FullscreenToggleScript::OnClick()
{
    AppEngine->GetEditorUIModule()->ToggleFullscreen();
}

FullscreenToggleScript::~FullscreenToggleScript()
{
    if (hasRegisteredCallback)
    {
        ButtonComponent* button = parent->GetComponent<ButtonComponent*>();
        if (button) button->RemoveOnClickCallback(delegateID);
    }
}

void FullscreenToggleScript::OnDestroy()
{
    hasRegisteredCallback = false;
    delegateID            = {};
}
