#pragma once

#include "Character.h"

class AudioSourceComponent;
class MeshComponent;
class ParticleSystemComponent;
class GameObject;
class AIAgentComponent;

enum class DestructibleStates
{
    NONE,
    NORMAL,
    DESTROYED,
    DESPAWNED
};

enum class DestructibleType
{
    VASE    = 0,
    BOX     = 1,
    CRYSTAL = 2,
};

class Destructible : public Script
{
  public:
    Destructible(GameObject* parent);
    ~Destructible() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;
    
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    void ValidateSetup();

  private:
    bool isSetupCorrectly           = false;
    bool isSimulating               = false;
    bool isWaitingToDespawn         = false;
    bool isDespawning               = false;

    DestructibleStates currentState = DestructibleStates::NONE;

    std::string destroyedMeshName;
    std::string destructionParticleSystemName;
    int destructibleTypeIndex                 = 1;

    MeshComponent* defaultMesh                = nullptr;
    GameObject* destroyedMesh                 = nullptr;
    ParticleSystemComponent* destructionSmoke = nullptr;

    float destructionSpawnDelay               = .1f;
    float destructionSpawnDelayCounter        = 0;
    float destructionDespawnDelay             = 4.0f;
    float destructionDespawnDelayCounter      = 0;
    float timeToDisappear                     = 3.0f;
    float disappearCounter                    = 0;

    DestructibleType type                     = DestructibleType::BOX;

    float origDestroyedMeshHeight             = 0.0f;

    // Audio
    AudioSourceComponent* audioComp           = nullptr;
};