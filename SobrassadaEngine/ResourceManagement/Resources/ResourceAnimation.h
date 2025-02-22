#pragma once
#include "Resource.h"
#include "Math/float3.h"
#include "Math/Quat.h"
#include <memory>
#include <unordered_map>

struct Channel
{
    std::unique_ptr<float3[]> positions;
    std::unique_ptr<float[]> posTimeStamps;
    std::unique_ptr<Quat[]> rotations;
    std::unique_ptr<float[]> rotTimeStamps;
    uint32_t numPositions = 0;
    uint32_t numRotations = 0;
};

class ResourceAnimation : public Resource
{
  public:
    std::unordered_map<std::string, Channel> channels;

    ResourceAnimation(UID uid, const std::string& name);
    ~ResourceAnimation() override;

    void SetDuration();
    float GetDuration() { return duration; }
    
  private:
    
    float duration = 0.0f;

};
