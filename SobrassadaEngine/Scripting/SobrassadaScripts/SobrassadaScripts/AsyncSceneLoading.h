#pragma once
#include "Script.h"

class VideoComponent;
class ChangeSceneScript;

class AsyncSceneLoading : public Script
{
  public:
    AsyncSceneLoading(GameObject* parent) : Script(parent) {}
    
    void OnDestroy() override;
    bool Init() override;
    void Update(float deltaTime) override;

private:

    bool isSetupCorrectly = true;

    VideoComponent* videoComponent = nullptr;
    ChangeSceneScript* changeSceneScript = nullptr;
    
};
