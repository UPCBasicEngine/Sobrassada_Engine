#pragma once
#include "Script.h"
#include "iostream"
#include <vector>
class CubeColliderComponent;
class CapsuleColliderComponent;
class ChangeSceneScript : public Script
{public:
    ChangeSceneScript(GameObject* parent);
    virtual ~ChangeSceneScript() noexcept override { parent = nullptr; }

    bool Init() override;
    void Update(float deltaTime) override {}
    void OnCollision(GameObject* otherObject, const float3& collisionNormal) override;

  private:
    std::string playerName   = "";
    const GameObject* player = nullptr;
    int indexScene           = 0;
    bool isOneUse            = false;
    std::vector<std::string> filesLoad;
    std::string scenesPath;
    int selectedLoad = -1;
    std::string fileName;
};
