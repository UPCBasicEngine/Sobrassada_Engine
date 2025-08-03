#pragma once

#include "Script.h"

class ShaderScriptComponent;
class MeshComponent;
class SphereColliderComponent;

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
    float activationRange                   = 10.0f;
    int damage                              = 1;
    float chargingDuration                  = 1.0f;
    float chargingTimer                     = 0.0f;
    float rotationSpeed                     = 90.0f;

    GameObject* character                   = nullptr;
    GameObject* decal                       = nullptr;
    GameObject* waterMesh                   = nullptr;
    ShaderScriptComponent* shaderScript     = nullptr;
    MeshComponent* shaderMesh               = nullptr;
    SphereColliderComponent* damageCollider = nullptr;

    ACTIVATION_STATE activationState        = SLEEPING;
};