#pragma once
#include "Script.h"

class GameObject;

class MenuChangeSceneScript : public Script
{
  public:
    MenuChangeSceneScript(GameObject* parent);
    virtual ~MenuChangeSceneScript() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;

  private:
    std::string targetSceneName = "";
    std::string scenesPath      = "";
    std::string fullScenePath   = "";
    bool sceneLoaded            = false;
};
