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
    void OnDestroy() override;

    void OnPlayerRespawn();
    void SetCachedGameStateID(AkUniqueID stateID) { cachedGameStateID = stateID; }
    void ResetToCachedGameState() const;

  private:
    bool isSetupCorrectly              = false;

    AkUniqueID cachedGameStateID = 0;

    // Audio
    AudioSourceComponent* audioComp    = nullptr;

    AkUniqueID levelStateAudioEvent  = 0;
    AkUniqueID gameStateAudioEvent = 0;
    AkUniqueID additionalAudioEvent  = 0;
};