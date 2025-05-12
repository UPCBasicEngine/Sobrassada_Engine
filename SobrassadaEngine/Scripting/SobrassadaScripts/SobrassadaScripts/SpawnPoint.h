#pragma once

#include "Script.h"

class GameObject;
class CharacterControllerComponent;

class SpawnPoint : public Script
{
  public:
    SpawnPoint(GameObject* parent);
    virtual ~SpawnPoint() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override {}
    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

  private:
    std::string playerName   = "";
    const GameObject* player = nullptr;

    bool isOneUse            = false;
};