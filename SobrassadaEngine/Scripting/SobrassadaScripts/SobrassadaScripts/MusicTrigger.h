#pragma once
#include "Script.h"

#include <AK/SoundEngine/Common/AkTypes.h>

class MusicManager;
class AudioSourceComponent;

class MusicTrigger : public Script
{
  public:
    MusicTrigger(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override {}
    void OnDestroy() override;

    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    bool isSetupCorrectly            = false;

    MusicManager* cachedMusicManager = nullptr;

    // Audio
    AudioSourceComponent* audioComp  = nullptr;

    AkUniqueID levelStateAudioEvent  = 0;
    AkUniqueID gameStateAudioEvent   = 0;
    AkUniqueID additionalAudioEvent  = 0;
};