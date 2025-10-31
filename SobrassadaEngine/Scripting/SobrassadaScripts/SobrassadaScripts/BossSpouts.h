#pragma once
#include "Math/float3.h"
#include "Script.h"

class BossSpouts : public Script
{
  public:
    BossSpouts(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    float3 center        = float3(0.0f, 0.0f, 0.0f); 
    float radius         = 5.0f;                 
    float speed          = 1.0f;                 
    float verticalOffset = 0.0f;                
    float angle          = 0.0f; 
    float uvAngle        = 0.0f;
    GameObject* spout    = nullptr;
};