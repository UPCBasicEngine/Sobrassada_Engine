

#include "pch.h"

#include "GameObject.h"
#include "NameDisplay.h"

#include "ShaderScriptComponent.h"
#include "UIFadeInOut.h"
#include "Standalone/Audio/AudioSourceComponent.h"

#include <glew.h>
#include <Math/FloatCmp.h>

NameDisplay::NameDisplay(GameObject* parent) : Script(parent)
{
    fields.emplace_back("Show automatically", InspectorField::FieldType::Bool, &showAutomatically);
    fields.emplace_back("Show delay", InspectorField::FieldType::Float, &showDelay, 0, 10);
    fields.emplace_back("Second show delay", InspectorField::FieldType::Float, &secondShowDelay, 0, 10);
    fields.emplace_back("Show duration", InspectorField::FieldType::Float, &showDuration, 1, 10);
    fields.emplace_back("Audio to play when showing", InspectorField::FieldType::Audio, &showAudio);
}

bool NameDisplay::Init()
{
    if (parent->GetChildren().size() < 2)
    {
        isSetupCorrectly = false;
        GLOG("Name display needs at least two children")
        return false;
    }

    audioComp = parent->GetComponent<AudioSourceComponent*>();
    if (audioComp == nullptr)
    {
        isSetupCorrectly = false;
        GLOG("[ERROR] Script parent does not contain an audio component")
        return false;
    }

    backgroundGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[0]);
    foregroundGO = AppEngine->GetSceneModule()->GetScene()->GetGameObjectByUID(parent->GetChildren()[1]);
    
    if (backgroundGO->GetComponent<ShaderScriptComponent*>() != nullptr)
        backgroundFade = backgroundGO->GetComponent<ShaderScriptComponent*>()->GetScriptByType<UIFadeInOut>();

    if (foregroundGO->GetComponent<ShaderScriptComponent*>() != nullptr)
        foregroundFade = foregroundGO->GetComponent<ShaderScriptComponent*>()->GetScriptByType<UIFadeInOut>();

    if (backgroundFade == nullptr) backgroundGO->SetEnabled(false);
    if (foregroundFade == nullptr) foregroundGO->SetEnabled(false);

    if (showAutomatically) ShowWithDelay();

    return true;
}

void NameDisplay::Update(float deltaTime)
{
    if (!isSetupCorrectly || currentState == NameDisplayStates::SHOWED) return;

    switch (currentState) {
    case NameDisplayStates::SHOW_DELAY:
        if (timer >= showDelay) Show();
        break;
    case NameDisplayStates::BACKGROUND_SHOWING:
        if (timer >= secondShowDelay)
        {
            timer = 0.0f;
            currentState = NameDisplayStates::FOREGROUND_SHOWING;
            foregroundGO->SetEnabled(true);
            if (foregroundFade != nullptr)
                foregroundFade->FadeIn();
        }
        break;
    case NameDisplayStates::FOREGROUND_SHOWING:
        if (timer >= showDuration)
        {
            Hide();
        }
            break;
    case NameDisplayStates::BACKGROUND_HIDING:
        if (timer >= secondShowDelay)
        {
            currentState = NameDisplayStates::SHOWED;
            if (backgroundFade != nullptr) backgroundFade->FadeOut();
            else backgroundGO->SetEnabled(false);
        }
            break;
    case NameDisplayStates::NONE:
    case NameDisplayStates::SHOWED:
        return;
    }

    timer += deltaTime;
}

void NameDisplay::ShowWithDelay()
{
    if (isSetupCorrectly)
    {
        timer = 0;
        currentState = NameDisplayStates::SHOW_DELAY;
    } else currentState = NameDisplayStates::NONE;
}

void NameDisplay::Show()
{
    if (isSetupCorrectly)
    {
        timer = 0.0f;

        
        backgroundGO->SetEnabled(true);
        if (backgroundFade != nullptr)
            backgroundFade->FadeIn();
        if (Equal(secondShowDelay, 0.0f))
        {
            currentState = NameDisplayStates::FOREGROUND_SHOWING;
            foregroundGO->SetEnabled(true);
            if (foregroundFade != nullptr)
                foregroundFade->FadeIn();
        } else currentState = NameDisplayStates::BACKGROUND_SHOWING;
            
        if (showAudio != 0) audioComp->EmitEvent(showAudio);
    } else currentState = NameDisplayStates::NONE;
}

void NameDisplay::Hide()
{
    timer = 0.0f;

    if (foregroundFade != nullptr) foregroundFade->FadeOut();
    else foregroundGO->SetEnabled(false);
    
    if (Equal(secondShowDelay, 0.0f))
    {
        currentState = NameDisplayStates::SHOWED;
        if (backgroundFade != nullptr) backgroundFade->FadeOut();
        else backgroundGO->SetEnabled(false);
    } else currentState = NameDisplayStates::BACKGROUND_HIDING;
}
