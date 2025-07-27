#pragma once

#include "HashString.h"
#include "Script.h"

class InputModule;
class MovingUVLight;
class MovingUVPostScript;
class ShaderScriptComponent;

class SwitchScriptTest : public Script
{
  public:
    SwitchScriptTest(GameObject* parent);
    ~SwitchScriptTest() override;

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    InputModule* inputModule               = nullptr;
    ShaderScriptComponent* shaderComponent = nullptr;
};
