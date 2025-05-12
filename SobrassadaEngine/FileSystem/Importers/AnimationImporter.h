#pragma once

#include "Globals.h"



namespace tinygltf
{
    class Model;
    struct Animation;
    struct AnimationSampler;
    struct Accessor;
    struct BufferView;
    struct Buffer;
} // namespace tinygltf
class ResourceAnimation;

enum class  AnimationType : uint8_t
{   TRANSLATION,
    ROTATION,
    SCALE
};
namespace AnimationImporter
{
    UID ImportAnimation(
        const tinygltf::Model& model, const tinygltf::Animation& animation, const std::string& name,
        const char* sourceFilePath, const std::string& targetFilePath, UID sourceUID, int animationIndex
    );
    ResourceAnimation* LoadAnimation(UID animationUID);
   
}; 