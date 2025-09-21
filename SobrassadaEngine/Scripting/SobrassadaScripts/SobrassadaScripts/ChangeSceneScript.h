#pragma once
#include "Script.h"

class GameObject;

class ChangeSceneScript : public Script
{
  public:
    ChangeSceneScript(GameObject* parent);
    virtual ~ChangeSceneScript() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override {}
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;
    void SwitchScene() const;

    const std::string& GetFullScenePath() const { return fullScenePath; }

  private:
    std::string playerName      = "";
    std::string targetSceneName = "";
    std::string scenesPath      = "";
    std::string fullScenePath   = "";
    GameObject* player          = nullptr;
};