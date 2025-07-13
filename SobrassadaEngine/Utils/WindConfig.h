#pragma once

#include "Globals.h"

#include "Math/float3.h"
#include "Math/float4.h"
#include "rapidjson/document.h"
#include <memory>

class WindConfig
{
  public:

    void SaveData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const;
    void LoadData(const rapidjson::Value& wind);

    bool GetApplyWindGlobally() const { return applyWindGlobally; }

    const float3& GetWindDirection() const { return windDirection; }

    float GetWindSpeed() const { return windSpeed; }

    float GetGustFrequency() const { return gustFrequency; }

    float GetGustSpeed() const { return gustSpeed; }

    bool& GetApplyWindGloballyRef() { return applyWindGlobally; }

    float3& GetWindDirectionRef() { return windDirection; }

    float& GetWindSpeedRef() { return windSpeed; }

    float& GetGustFrequencyRef() { return gustFrequency; }

    float& GetGustSpeedRef() { return gustSpeed; }

private:

    bool applyWindGlobally = false;
    float3 windDirection = float3::zero;
    float windSpeed = 0.f;
    float gustFrequency = 0.f;
    float gustSpeed = 0.f;
    
};