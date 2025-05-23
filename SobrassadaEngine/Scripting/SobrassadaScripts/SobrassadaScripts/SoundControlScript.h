#pragma once
#include "Script.h"

class SoundControlScript : public Script
{
  public:
    SoundControlScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

  private:
    float minDistanceToPlayer = 10.0f;
    float maxVolume           = 1.0f;
    float3 finalPosition;
    bool isActive = false;
};