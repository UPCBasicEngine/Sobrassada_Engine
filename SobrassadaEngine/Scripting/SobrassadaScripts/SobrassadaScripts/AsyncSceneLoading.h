#pragma once
#include "Script.h"

class VideoComponent;

class AsyncSceneLoading : public Script
{
  public:
    AsyncSceneLoading(GameObject* parent);

    void OnDestroy() override {}
    bool Init() override;
    void Update(float deltaTime) override;

  private:

    bool SkipCutscene() const;

private:
    std::string targetSceneName;
    bool useAsyncLoading = true;
    float minimumPlayTimeBeforeSkip = 1.0f;

    std::string fullScenePath;
    bool isSetupCorrectly          = true;

    VideoComponent* videoComponent = nullptr;
};
