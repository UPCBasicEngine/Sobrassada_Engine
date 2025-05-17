#pragma once
#include "Script.h"

#include "Math/Quat.h"

class TileFloatScript : public Script
{
  public:
    TileFloatScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) override;
    void Load(const rapidjson::Value& initialState) override;
    inline float QuaternionDot(const Quat& a, const Quat& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
  private:
    int speed = 10;
    float3 finalPosition;
    Quat finalRotation;
    Quat startQuat;
    float3 finalScale;
    float3 startPosition;
    float3 startRotation;
    float3 startScale = float3(1.0f,1.0f,1.0f);
    float minDistanceToPlayer = 10.0f;
    bool isActive = false;
};