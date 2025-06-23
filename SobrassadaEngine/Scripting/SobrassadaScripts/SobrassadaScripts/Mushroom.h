#pragma once

#include "Script.h"

class GameObject;
class MeshComponent;
class SphereColliderComponent;

class Mushroom : public Script
{
  public:
    Mushroom(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

    bool IsReady() const;
    void Disable();

  private:
    SphereColliderComponent* collider = nullptr;
    MeshComponent* mushroom           = nullptr;
};
