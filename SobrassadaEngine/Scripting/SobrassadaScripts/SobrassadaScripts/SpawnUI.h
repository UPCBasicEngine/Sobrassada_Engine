#pragma once

#include "Script.h"

class GameObject;
class SphereColliderComponent;

class SpawnUI : public Script
{
  public:
    SpawnUI(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

  private:
    SphereColliderComponent* trigger = nullptr;
    GameObject* objectUI             = nullptr;
    std::string objectUIName         = "";
    bool onCollision                 = false;
};
