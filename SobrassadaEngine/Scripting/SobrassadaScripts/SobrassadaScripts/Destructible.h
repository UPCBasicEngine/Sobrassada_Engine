#pragma once

#include "Character.h"

class MeshComponent;
class ParticleSystemComponent;
class GameObject;
class AIAgentComponent;

enum class DestructibleStates
{
    NONE,
    NORMAL,
    DESTROYED
};

class Destructible : public Character
{
  public:
    Destructible(GameObject* parent);
    ~Destructible() noexcept override { parent = nullptr; };

    bool Init() override;
    void Update(float deltaTime) override;

    void OnDeath() override;

  private:
    void ValidateSetup();

  private:
    bool isSetupCorrectly           = false;

    DestructibleStates currentState = DestructibleStates::NONE;

    std::string destroyedMeshName;
    std::string destructionParticleSystemName;

    MeshComponent* defaultMesh                = nullptr;
    GameObject* destroyedMesh                 = nullptr;
    ParticleSystemComponent* destructionSmoke = nullptr;

    float timeToDisappear                     = 5;
    float disappearCounter                    = 0;
};