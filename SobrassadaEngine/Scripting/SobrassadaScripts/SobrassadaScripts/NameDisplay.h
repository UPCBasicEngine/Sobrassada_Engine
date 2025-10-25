#pragma once
#include "Script.h"

#include <AK/SoundEngine/Common/AkTypes.h>

class UIFadeInOut;

enum class NameDisplayStates
{
    NONE,
    SHOW_DELAY,
    BACKGROUND_SHOWING,
    FOREGROUND_SHOWING,
    BACKGROUND_HIDING,
    SHOWED
};

class NameDisplay : public Script
{
  public:
    NameDisplay(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override;

    void ShowWithDelay();

  private:
    void Show();
    void Hide();

  private:
    NameDisplayStates currentState = NameDisplayStates::NONE;
    
    bool isSetupCorrectly  = true;

    bool showAutomatically = false;
    float showDelay        = 2.0f;
    float secondShowDelay  = .2f;
    float showDuration     = 5.0f;

    float timer      = 0.0f;

    GameObject* backgroundGO = nullptr;
    GameObject* foregroundGO = nullptr;
    UIFadeInOut* backgroundFade;
    UIFadeInOut* foregroundFade;
    
    // Audio
    AudioSourceComponent* audioComp           = nullptr;
    AkUniqueID showAudio = 0;
};