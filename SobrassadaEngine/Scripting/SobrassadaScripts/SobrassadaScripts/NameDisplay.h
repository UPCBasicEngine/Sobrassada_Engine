#pragma once
#include "Script.h"


class NameDisplay : public Script
{
  public:
    NameDisplay(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override;

    void ShowWithDelay();

  private:

    void Show();

private:

    bool isSetupCorrectly = true;
    bool childrenVisible = false;
    bool showed = true;

    bool showAutomatically = false;
    float showDelay = 2.0f;
    float showDuration = 5.0f;

    float showCounter = 0.0f;
};