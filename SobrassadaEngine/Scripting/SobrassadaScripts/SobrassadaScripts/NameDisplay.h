#pragma once
#include "Script.h"


class NameDisplay : public Script
{
  public:
    NameDisplay(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override;

    void Show();

  private:

    bool isSetupCorrectly = true;
    bool childrenVisible = false;
    bool showed = false;

    bool showAutomatically = false;
    float showDelay = 2.0f;
    float showDuration = 5.0f;

    float showCounter = 0.0f;
};