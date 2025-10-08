#pragma once
#include "HashString.h"
#include "Script.h"

#include <AK/SoundEngine/Common/AkTypes.h>

class AudioSourceComponent;
class GameObject;

class MagicBarrier : public Script
{
  public:
    MagicBarrier(GameObject* parent);

    bool Init() override;
    void Update(float deltaTime) override {}

    void RegisterEnemy() { enemiesInArea++; }
    void EnemyDied();

    int GetEnemiesInArea() const { return enemiesInArea; }

  private:
    std::string areaTagString;
    HashString areaTag;
    int enemiesInArea               = 0;

    // Audio
    AudioSourceComponent* audioComp = nullptr;

    AkUniqueID gameStateAudioEvent  = 0;
};
