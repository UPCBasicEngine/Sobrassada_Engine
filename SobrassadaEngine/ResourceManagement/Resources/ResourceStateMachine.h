#pragma once

#include "HashString.h"
#include "Resource.h"

#include "Math/float2.h"
#include "imgui.h"
#include "../Components/Standalone/Animation/TriggerType.h"
#include <string>
#include <vector>

struct Clip
{
    UID animationResourceUID;
    HashString clipName;
    bool loop;
    float animationSpeed = 1.0f;
};

struct StateTrigger
{
    float keyTimeNorm;
    TriggerType type;
    std::string eventName;
    bool repeatOnLoop = true;
    bool consumed     = false;
};

struct State
{
    HashString name;
    HashString clipName;
    ImVec2 position;
    std::vector<StateTrigger> triggers;
    
};

struct Transition
{
    HashString fromState;
    HashString toState;
    HashString triggerName;
    unsigned interpolationTime;
};

class SOBRASADA_API_ENGINE ResourceStateMachine : public Resource
{
  public:
    ResourceStateMachine(UID uid, const std::string& name);
    ~ResourceStateMachine() override = default;

    void AddClip(UID animationResourceUID, const std::string& name, bool loop);
    bool RemoveClip(const std::string& name);
    bool EditClipInfo(const std::string& oldName, UID newUID, const std::string& newName, bool newLoop, float newSpeed);
    bool ClipExists(const std::string& clipName) const;

    void AddState(const std::string& stateName, const std::string& clipName);
    bool RemoveState(const std::string& stateName);
    bool EditState(const std::string& oldStateName, const std::string& newStateName, const std::string& newClipName);

    void AddTransition(
        const std::string& fromState, const std::string& toState, const std::string& trigger, unsigned interpolationTime
    );
    bool RemoveTransition(const std::string& fromState, const std::string& toState);
    bool EditTransition(
        const std::string& fromState, const std::string& toState, const std::string& newTrigger,
        unsigned newInterpolationTime
    );

    bool UseTrigger(const std::string& triggerName, const State*& currentAnimState);

    const Clip* GetClip(const std::string& name) const;
    void SetClipSpeed(const std::string& name,float speed);
    State* GetState(const std::string& name);
    const Transition* GetTransition(const std::string& fromState, const std::string& toState) const;
    const State* GetDefaultState() const
    {
        if (defaultStateIndex >= 0 && defaultStateIndex < (int)states.size()) return &states[defaultStateIndex];
        return nullptr;
    }

    void ChangeCurrentState(int newStateIndex, const State*& currentState);

    void SetDefaultState(int state) { defaultStateIndex = state; }
    void ResetClipsSpeed();

  public:
    std::vector<Clip> clips;
    std::vector<State> states;
    std::vector<Transition> transitions;
    std::vector<std::string> availableTriggers;
    std::vector<float> clipsDefaultSpeed;

  private:
    int defaultStateIndex = -1;
};