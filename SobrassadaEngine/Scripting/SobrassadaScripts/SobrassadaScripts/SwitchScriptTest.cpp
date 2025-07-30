#include "pch.h"

#include "GameObject.h"
#include "InputModule.h"
#include "MovingUVLight.h"
#include "MovingUVPostScript.h"
#include "ScriptComponent.h"
#include "ShaderScriptComponent.h"
#include "SwitchScriptTest.h"

SwitchScriptTest::SwitchScriptTest(GameObject* parent) : Script(parent)
{
}

SwitchScriptTest::~SwitchScriptTest()
{
}

bool SwitchScriptTest::Init()
{
    shaderComponent = parent->GetComponent<ShaderScriptComponent*>();

    inputModule     = AppEngine->GetInputModule();

    if (shaderComponent)
    {
        shaderComponent->SetScriptEnabled("MovingUVLight", true);
        shaderComponent->SetScriptEnabled("MovingUVPostScript", false);
    }

    return true;
}

void SwitchScriptTest::Update(float deltaTime)
{
    // ONLY THING IT DOES IS ENABLE / DISABLE SHADER SCRIPT TO CHECK IT WORKS HAVING MULTIPLE SCRITS
    if (!inputModule || !shaderComponent) return;

    const KeyState* keyboard = inputModule->GetKeyboard();

    if (keyboard[SDL_SCANCODE_6] == KeyState::KEY_DOWN)
    {
        shaderComponent->SetScriptEnabled("MovingUVLight", true);
        shaderComponent->SetScriptEnabled("MovingUVPostScript", false);
    }
    else if (keyboard[SDL_SCANCODE_7] == KeyState::KEY_DOWN)
    {
        shaderComponent->SetScriptEnabled("MovingUVLight", false);
        shaderComponent->SetScriptEnabled("MovingUVPostScript", true);
    }
}
