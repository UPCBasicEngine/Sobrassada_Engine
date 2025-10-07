#pragma once
#include "Script.h"

#include <AK/SoundEngine/Common/AkTypes.h>

class AudioSourceComponent;

class MusicManager : public Script
{
  public:
    MusicManager(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override {}

    void OnPlayerRespawn() const;

  private:
    bool isSetupCorrectly              = false;

    // Audio
    AudioSourceComponent* audioComp    = nullptr;

    AkUniqueID firstRespawnAudioEvent  = 0;
    AkUniqueID secondRespawnAudioEvent = 0;
    AkUniqueID thirdRespawnAudioEvent  = 0;
};