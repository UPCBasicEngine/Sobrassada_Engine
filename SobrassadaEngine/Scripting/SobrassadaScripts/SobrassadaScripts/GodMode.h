#pragma once

#include "Script.h"

#include "Math/float4x4.h"

class GameObject;
class CharacterControllerComponent;
class CameraComponent;

class GodMode : public Script
{
  public:
    GodMode(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

  private:
    CharacterControllerComponent* characterController = nullptr;
    std::string cameraName                            = "";
    CameraComponent* godCamera                        = nullptr;
    bool freeCamera                                   = false;
    float4x4 originalTransform;
};