#pragma once

#include "Script.h"

class MeshComponent;
class MirageBossDash;

enum class MirageState
{
    Sleeping,
    Warning,
    Damaging
};
class Mirage : public Script
{
  public:
    Mirage(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

  protected:
    MirageState state            = MirageState::Sleeping;
    UID mirageWarningImage       = 0;
    UID mirageDamageImage        = 0;

    MeshComponent* meshComponent = nullptr;
    int damage                   = 0;
    float warningDelay           = 0.f;
    float damageDuration         = 0.f;
    float stateTimer             = 0.0f;
    int weightOrder              = 0;

    const float3* endPoint     = nullptr;
    MirageBossDash* bossDash     = nullptr;
};