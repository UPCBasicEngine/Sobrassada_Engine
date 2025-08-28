#pragma once
#include "Script.h"

class GameObject;

class WallCollision : public Script
{
  public:
    WallCollision(GameObject* parent);
    virtual ~WallCollision() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override {}
    void OnCollision(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    // No necesita variables adicionales
};