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
    int getOrder() { return weightOrder; }

  protected:
    MirageState state            = MirageState::Sleeping;


    MeshComponent* meshComponent = nullptr;
    MeshComponent* mirageDisableComponent1 = nullptr;
    MeshComponent* mirageDisableComponent2 = nullptr;
    int damage                   = 0;
    float warningDelay           = 0.f;
    float damageDuration         = 0.f;
    float stateTimer             = 0.0f;
    int weightOrder              = 0;

    float3 endPoint;
    MirageBossDash* bossDash     = nullptr;
};