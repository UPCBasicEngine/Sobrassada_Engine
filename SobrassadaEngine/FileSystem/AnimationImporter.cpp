#include "AnimationImporter.h"
#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace AnimationImporter
{
    UID ImportAnimation(
        const tinygltf::Model& model, const tinygltf::Animation& animation, const std::string& name,
        const char* filePath
    )
    {
        for (const auto& channel : animation.channels)
        {
            const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];

            const tinygltf::Accessor& input           = model.accessors[sampler.input];
            const tinygltf::Accessor& output           = model.accessors[sampler.output];

             const tinygltf::BufferView& inputView      = model.bufferViews[input.bufferView];
            const tinygltf::BufferView& outputView     = model.bufferViews[output.bufferView];

            const tinygltf::Buffer& inputBuffer         = model.buffers[inputView.buffer];
            const tinygltf::Buffer& outputBuffer        = model.buffers[outputView.buffer];

            const float* timeStamps =
                reinterpret_cast<const float*>(&inputBuffer.data[input.byteOffset + inputView.byteOffset]);
            const float* values =
                reinterpret_cast<const float*>(&outputBuffer.data[output.byteOffset + outputView.byteOffset]);

            size_t keyframeCount = input.count;

            for (size_t i = 0; i < keyframeCount; ++i)
            {
                GLOG("Timestamp: %f", timeStamps[i]);
            }
            
        }
    }
}