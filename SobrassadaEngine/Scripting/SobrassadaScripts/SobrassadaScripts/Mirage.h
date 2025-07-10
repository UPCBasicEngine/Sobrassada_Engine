#pragma once

#include "Script.h"

enum ACTIVATION_STATE
{
    SLEEPING,
    WARNING,
    DAMAGING,
};

enum ShapeType
{
    Circle,
    Rectangle
};

class Mirage : public Script
{
    void Update(float deltaTime) override;
    Mirage(GameObject* parent);

  protected:
    UID mirageWarningImage = 0;
    UID mirageDamageImage  = 0;

    int damage             = 0;
    float warningDelay     = 0.f;
    float damageDuration   = 0.f;

    float sizeX            = 2.0f;
    float sizeY            = 2.0f;
    float rotation         = 0.0f;
};
