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

enum MirageState
{
    Idle,
    PlayingSequence,
};

class BossMirage : public Script
{
  public:
    BossMirage(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    void StartSequence(int sequence);

  private:
    int currentSequence = 1;
    AttackSequence sequence1, sequence2, sequence3;

    MirageState state               = MirageState::Idle;
    AttackSequence* sequence = nullptr;
    size_t currentMirageIndex       = 0;
    float timeSinceLastActivation   = 0.0f;
};
