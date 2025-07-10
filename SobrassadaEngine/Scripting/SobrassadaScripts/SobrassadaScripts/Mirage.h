#pragma once

#include "Script.h"

enum ACTIVATION_STATE
{
    SLEEPING,
    WARNING,
    DAMAGING,
};


class Mirage : public Script
{
    void Update(float deltaTime) override;
    Mirage(GameObject* parent);

  protected:
    UID mirageWarningImage     = 0;
    UID mirageDamageImage      = 0;

    int damage                 = 0;
    float warningDelay         = 0.f;
    float damageDuration       = 0.f;

    float3 startPosition, startRotation;
    float3 startScale          = float3(1.0f, 1.0f, 1.0f);

    GameObject* mirageCollider = nullptr;
};
