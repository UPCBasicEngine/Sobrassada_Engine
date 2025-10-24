#pragma once
#include "Script.h"

class UIFadeInOut;
class GameObject;

class ChangeSceneScript : public Script
{
  public:
    ChangeSceneScript(GameObject* parent);
    virtual ~ChangeSceneScript() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override;
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    std::string playerName      = "";
    std::string targetSceneName = "";
    std::string scenesPath      = "";
    std::string fullScenePath   = "";

    std::string fadeOutGameObjectName     = "SceneFadeOut";
    
    GameObject* player          = nullptr;
    UIFadeInOut* optionalFadeOutScript = nullptr;

    bool changeSceneTriggered = false;
    float timer;
};