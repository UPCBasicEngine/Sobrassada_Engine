#pragma once
#include <string>

enum class TriggerType
{
	SOUND = 0
};

class AnimationTrigger
{
public:

	AnimationTrigger(float atTime, TriggerType type, const std::string& eventName, bool repeat = true);

	bool Check(float prevTime, float currTime, bool looped);
    void Reset() { consumed = false; }
    void Consume() { consumed = true; }

    float GetTime() const { return keyTime; }
    void SetTime(float t) { keyTime = t; }
    void SetType(TriggerType t) { type = t; }
    void SetName(const std::string& p) { eventName = p; }

    TriggerType GetType() const { return type; }
    const std::string& GetName() const { return eventName; }

    bool RepeatOnLoop() const { return repeatOnLoop; }
    void SetRepeat(bool r) { repeatOnLoop = r; }

private:

	float keyTime = 0.0f;
	TriggerType type = TriggerType::SOUND;
    std::string eventName;
    bool consumed = false;
    bool repeatOnLoop = true;  

};
