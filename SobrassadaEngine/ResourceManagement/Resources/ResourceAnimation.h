#pragma once
#include "Resource.h"
#include "HashString.h"

#include "Math/float3.h"
#include "Math/Quat.h"
#include <unordered_map>

struct Channel
{
    // Position data
    std::vector<float> posTimeStamps;
    std::vector<float3> positions;
    uint32_t numPositions = 0;

    // Rotation data
    std::vector<float> rotTimeStamps;
    std::vector<Quat> rotations;
    uint32_t numRotations = 0;

    // Scale data
    std::vector<float> scaleTimeStamps;
    std::vector<float3> scales;
    uint32_t numScales = 0;
};

class ResourceAnimation : public Resource
{
  public:
    ResourceAnimation(UID uid, const std::string& name);
    ~ResourceAnimation() override;

    void SetDuration();
    float GetDuration() { return duration; }
    Channel* GetChannel(const HashString& nodeName);

  public:
    std::unordered_map<HashString, Channel> channels;
    std::vector<HashString> channelNames;
    
  private:
    
    float duration = 0.0f;

};
