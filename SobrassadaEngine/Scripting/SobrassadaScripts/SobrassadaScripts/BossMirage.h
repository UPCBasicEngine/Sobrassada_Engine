#pragma once

#include "Script.h"

class Mirage;

struct AttackSequence
{
    std::vector<Mirage*> mirageZones;
    float delayBetweenZones = 1.0f;
};


struct SequenceTrigger
{
    float hpThreshold;
    AttackSequence sequence;
};

class BossMirage : public Script
{
    bool Init() override;
    void Update(float deltaTime) override;


  private:
    int currentPhase = 0;
};
