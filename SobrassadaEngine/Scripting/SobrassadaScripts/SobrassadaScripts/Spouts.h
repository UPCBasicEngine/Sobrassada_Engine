#pragma once

#include "Script.h"

class ShaderScriptComponent;
class MeshComponent;
class SphereColliderComponent;
class ParticleSystemComponent;

class Spouts : public Script
{
    enum ACTIVATION_STATE
    {
        SLEEPING,
        CHARGING,
        DAMAGING,
        COOLDOWN
    };

  public:
    Spouts(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    int GetDamage() { return damage; }

  private:
    bool enableRune                         = false;
    float activationRange                   = 10.0f;
    int damage                              = 1;
    float chargingDuration                  = 1.0f;
    float chargingTimer                     = 0.01f;
    float spoutWaterTimer                   = 0.01f;
    float rotationSpeedWhiteWaves           = 90.0f;
    float minScaleTornado                   = 0.1f;
    float maxScaleTornado                   = 12.0f;
    float initialScaleTornado               = 9.0f;
    float rotationSpeedTornado              = 90.0f;
    float rotationSpeedBlueWaves            = 90.0f;
    float explosionDuration                 = 0.01f;

    GameObject* character                   = nullptr;
    GameObject* whiteWaves                  = nullptr;
    GameObject* tornadoWater                = nullptr;
    GameObject* blueWaves                   = nullptr;
    GameObject* explosion                   = nullptr;
    GameObject* waterMesh                   = nullptr;
    GameObject* particleGO                  = nullptr;
    GameObject* rune                        = nullptr;

    ShaderScriptComponent* whiteWavesScript = nullptr;
    MeshComponent* shaderwhiteWavesMesh     = nullptr;

    ShaderScriptComponent* shaderScript     = nullptr;
    MeshComponent* shaderWaterMesh          = nullptr;

    ShaderScriptComponent* explosionScript  = nullptr;
    MeshComponent* shaderExplosionMesh      = nullptr;

    SphereColliderComponent* damageCollider = nullptr;
    ParticleSystemComponent* particles      = nullptr;

    ACTIVATION_STATE activationState        = SLEEPING;
};