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

  private:
    float speed = 50.0f;
    float3 finalPosition;
    Quat finalRotation;
    float3 finalScale;
    float3 startPosition;
    float3 startRotation;
    float3 startScale;
    bool isActive = false;
};