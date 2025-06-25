#include "AnimationTrigger.h"

AnimationTrigger::AnimationTrigger(float atTime, TriggerType type, const std::string& eventName)
    : keyTime(atTime), type(type), eventName(eventName)
{
}

bool AnimationTrigger::Check(float prevTime, float currTime, bool looped)
{
    if (consumed && !looped) return false;

    bool crossed = (!consumed && prevTime < keyTime && currTime >= keyTime) || (looped && currTime >= keyTime);

    if (crossed) consumed = true;
    if (looped) consumed = (currTime < keyTime);

    return crossed;
}

void AnimationTrigger::Reset()
{
    consumed = false;
}