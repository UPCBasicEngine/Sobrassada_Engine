#pragma once
#include "Script.h"

class AnimationComponent;

class BasicAnimationController : public Script
{
  public:

    BasicAnimationController(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override {}

  protected:
    AnimationComponent* animComponent           = nullptr;
    
};