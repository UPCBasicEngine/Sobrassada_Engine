#pragma once

#include "Script.h"

class CubeColliderComponent;

class Mirage : public Script
{
  public:
    Mirage(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

  protected:
    UID mirageWarningImage = 0;
    UID mirageDamageImage  = 0;

    int damage             = 0;
    float warningDelay     = 0.f;
    float damageDuration   = 0.f;
    int weightOrder        = 0;
};
