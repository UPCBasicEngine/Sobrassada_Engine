#include "ResourceAnimation.h"

#include "Application.h"
#include "LibraryModule.h"
#include "TextureImporter.h"

#include <glew.h>

#include "imgui.h"
#include "DirectXTex/DirectXTex.h"
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

ResourceAnimation::ResourceAnimation(UID uid, const std::string& name) : Resource(uid, name, ResourceType::Animation)
{
}

ResourceAnimation::~ResourceAnimation()
{
   
}


void ResourceAnimation::SetDuration()
{
    float maxTime = 0.0f;

    for (const auto& channel : channels)
    {
        if (channel.second.numPositions > 0)
        {
            float lastTime = channel.second.posTimeStamps[channel.second.numPositions - 1];
            maxTime        = std::max(maxTime, lastTime);
        }
        if (channel.second.numRotations > 0)
        {
            float lastTime = channel.second.rotTimeStamps[channel.second.numRotations - 1];
            maxTime        = std::max(maxTime, lastTime);
        }

        
    }

    duration = maxTime;
}

Channel* ResourceAnimation::GetChannel(const std::string& nodeName)
{
    auto it = channels.find(nodeName);
    if (it != channels.end())
    {
        return &it->second;
    }

    return nullptr; 
}