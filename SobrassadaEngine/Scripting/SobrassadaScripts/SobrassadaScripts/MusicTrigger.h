#pragma once
#include "Script.h"

#include <AK/SoundEngine/Common/AkTypes.h>

class AudioSourceComponent;

class MusicTrigger : public Script
{
  public:
    MusicTrigger(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override {}
    
    void OnCollisionEnter(GameObject* otherObject, const float3 collisionNormal, ColliderLayer layer) override;

  private:
    
    bool isSetupCorrectly               = false;
    
    // Audio
    AudioSourceComponent* audioComp = nullptr;

    AkUniqueID audioToEmit;
};