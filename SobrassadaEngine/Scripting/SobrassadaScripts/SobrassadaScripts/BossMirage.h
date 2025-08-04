#pragma once

#include "Script.h"

class Scene;

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

enum class SequenceState
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
    SequenceState GetSequenceState() { return state; };

    std::vector<GameObject*> GetMirageChildren(Scene* scene, const std::string& parentName);

  private:
    int currentSequence = 1;
    AttackSequence sequence1, sequence2, sequence3;

    SequenceState state           = SequenceState::Idle;
    AttackSequence* sequence      = nullptr;
    size_t currentMirageIndex     = 0;
    float timeSinceLastActivation = 0.0f;
};
