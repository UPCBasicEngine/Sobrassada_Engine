#pragma once
#include <string>

enum class TriggerType
{
	SOUND
};

class AnimationTrigger
{
public:

	AnimationTrigger(float atTime, TriggerType type, const std::string& payload);

	bool Check(float prevTime, float currTime, bool looped);
    void Reset();

    float GetTime() const { return keyTime; }
    void SetTime(float t) { keyTime = t; }

    TriggerType GetType() const { return type; }
    const std::string& GetData() const { return payload; }

private:

	float keyTime = 0.0f;
	TriggerType type = TriggerType::SOUND;
    std::string payload;
    bool consumed = false;

};
