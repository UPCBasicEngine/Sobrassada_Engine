#pragma once

#include "Script.h"

struct AttackSequence
{
    std::vector<GameObject*> mirageObjects;
    float delayBetweenZones = 1.0f;
};

struct SequenceTrigger
{
    float hpThreshold;
    AttackSequence sequence;
};

class BossMirage : public Script
{
  public:
    BossMirage(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;

  private:
    int currentSequence = 1;
    std::vector<AttackSequence> sequences;
};
