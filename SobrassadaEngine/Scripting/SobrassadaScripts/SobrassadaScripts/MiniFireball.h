#pragma once
#include "Script.h"

class MiniFireball : public Script
{
  public:
    MiniFireball(GameObject* parent) : Script(parent) {}

    int damage = 1;   
    float life = 3.f; 

    bool Init() override;
    void Update(float dt) override;
    void OnCollision(GameObject* other, const float3 normal, ColliderLayer layer) override;

  private:
    float lifeTimer = 0.f;
    GameObject* shadow = nullptr;
    float3 baseScale   = float3::one;
};
