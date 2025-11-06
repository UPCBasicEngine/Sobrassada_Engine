#pragma once

#include "Script.h"

class ParticleSystemComponent;
class GameObject;
class CharacterControllerComponent;

class SpawnPoint : public Script
{
  public:
    SpawnPoint(GameObject* parent);
    virtual ~SpawnPoint() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override {}
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    bool isSetupCorrectly    = true;
    std::string playerName   = "";
    std::string treeName     = "EN_TreeOfLife";
    std::string leafsName    = "Tree_Leaves";
    std::string particleName = "TOL_Leaf";
    const GameObject* player = nullptr;
    bool isOneUse            = false;
    int setHealth            = 0;
    bool activated           = false;
    GameObject* tree        = nullptr;
    GameObject* leafs        = nullptr;
    ParticleSystemComponent* particleSystem     = nullptr;
};