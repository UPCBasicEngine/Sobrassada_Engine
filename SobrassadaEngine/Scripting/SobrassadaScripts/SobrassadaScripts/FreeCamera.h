#pragma once

#include "Script.h"

class CameraComponent;

class FreeCamera : public Script
{
  public:
    FreeCamera(GameObject* parent) : Script(parent) {}
    bool Init() override;
    void Update(float deltaTime) override;

  private:
    CameraComponent* camera = nullptr;
};
