#pragma once
#include "Delegate.h"
#include "Script.h"
#include <list>
#include <string>

class GameObject;

class MenuChangeSceneScript : public Script
{
  public:
    MenuChangeSceneScript(GameObject* parent);
    virtual ~MenuChangeSceneScript() noexcept override;

    bool Init() override;
    void Update(float deltaTime) override;
    void OnDestroy() override;

    void OnClick();

  private:
    std::string targetSceneName = "";
    std::string scenesPath      = "";
    std::string fullScenePath   = "";
    bool sceneLoaded            = false;

    std::list<Delegate<void>>::iterator delegateID;
    bool hasRegisteredCallback = false;
};