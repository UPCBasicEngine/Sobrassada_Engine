#include "AnimationTrigger.h"

AnimationTrigger::AnimationTrigger(float atTime, TriggerType type, const std::string& payload)
    : keyTime(atTime), type(type), payload(payload)
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