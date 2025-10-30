#pragma once

#include "Script.h"

class MeshComponent;
class MirageBossDash;
class AudioSourceComponent;
class ShaderScriptComponent;
class MirageVFX;

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
    MirageState state                          = MirageState::Sleeping;

    MeshComponent* meshComponent               = nullptr;
    MeshComponent* mirageBorder                = nullptr;
    MeshComponent* mirageDisableComponent2     = nullptr;
    MeshComponent* mirageArrow                 = nullptr;
    ShaderScriptComponent* mirageFireComponent = nullptr;

    bool dashdone                              = false;
    int damage                                 = 0;
    float warningDelay                         = 0.f;
    float damageDuration                       = 0.f;
    float stateTimer                           = 0.0f;
    int weightOrder                            = 0;

    float3 endPoint;
    MirageBossDash* bossDash    = nullptr;
    MirageVFX* firescript       = nullptr;
    AudioSourceComponent* audio = nullptr;

    UID matMirageArrowBlue      = INVALID_UID;
    UID matMirageBorderBlue     = INVALID_UID;
};