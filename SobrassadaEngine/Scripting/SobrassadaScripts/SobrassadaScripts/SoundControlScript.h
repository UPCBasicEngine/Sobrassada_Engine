#pragma once
#include "Script.h"

class SoundControlScript : public Script
{
public:
    SoundControlScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    void DynamicSoundModifier();
private:
    float minDistanceToPlayer = 10.0f;
};