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

        std::vector<char> buffer;

        // Guarda número de canales
        uint32_t channelCount = static_cast<uint32_t>(animation.channels.size());
        buffer.insert(
            buffer.end(), reinterpret_cast<char*>(&channelCount),
            reinterpret_cast<char*>(&channelCount) + sizeof(uint32_t)
        );

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

            std::string nodeName = model.nodes[channel.target_node].name;
            uint32_t nameSize    = static_cast<uint32_t>(nodeName.size());
            //GLOG("Writing node name: %s, size = %u", nodeName.c_str(), nameSize);
            buffer.insert(
                buffer.end(), reinterpret_cast<char*>(&nameSize), reinterpret_cast<char*>(&nameSize) + sizeof(uint32_t)
            );
            buffer.insert(buffer.end(), nodeName.begin(), nodeName.end());

            uint32_t animType = 0; // 0 = Translation, 1 = Rotation

            if (channel.target_path == "rotation")
            {
                animType = 1;
            }

            buffer.insert(
                buffer.end(), reinterpret_cast<char*>(&animType), reinterpret_cast<char*>(&animType) + sizeof(uint32_t)
            );

            buffer.insert(
                buffer.end(), reinterpret_cast<char*>(&keyframeCount),
                reinterpret_cast<char*>(&keyframeCount) + sizeof(uint32_t)
            );

            buffer.insert(
                buffer.end(), reinterpret_cast<const char*>(timeStamps),
                reinterpret_cast<const char*>(timeStamps) + keyframeCount * sizeof(float)
            );

            if (channel.target_path == "translation")
            {
                buffer.insert(
                    buffer.end(), reinterpret_cast<const char*>(values),
                    reinterpret_cast<const char*>(values) + keyframeCount * sizeof(float3)
                );
            }
            else if (channel.target_path == "rotation")
            {
                buffer.insert(
                    buffer.end(), reinterpret_cast<const char*>(values),
                    reinterpret_cast<const char*>(values) + keyframeCount * sizeof(Quat)
                );
            }
            
        }
        UID animationUID     = GenerateUID();

        std::string savePath = ANIMATIONS_PATH + std::string("Animation") + ANIMATION_EXTENSION;
        UID finalAnimationUID = App->GetLibraryModule()->AssignFiletypeUID(animationUID, savePath);
        std::string fileName = FileSystem::GetFileNameWithoutExtension(filePath);

        savePath              = ANIMATIONS_PATH + std::to_string(finalAnimationUID) + ANIMATION_EXTENSION;

        FileSystem::Save(savePath.c_str(), buffer.data(), buffer.size(), true);

        App->GetLibraryModule()->AddAnimation(animationUID, name);
        App->GetLibraryModule()->AddResource(savePath, animationUID);

        GLOG("%s saved as binary", fileName.c_str());

        LoadAnimation(animationUID);

        return animationUID;
    }

    ResourceAnimation* LoadAnimation(UID animationUID)
    {
        std::string path      = App->GetLibraryModule()->GetResourcePath(animationUID);
        char* buffer          = nullptr;

        unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);
        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load animation file: %s", path.c_str());
            return nullptr;
        }

        char* cursor = buffer;

        uint32_t channelCount;
        memcpy(&channelCount, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);

        ResourceAnimation* animation =
            new ResourceAnimation(animationUID, FileSystem::GetFileNameWithoutExtension(path));

        for (uint32_t i = 0; i < channelCount; ++i)
        {
            Channel animChannel;
            uint32_t nameSize;
            memcpy(&nameSize, cursor, sizeof(uint32_t));
            cursor += sizeof(uint32_t);

            std::string nodeName(cursor, nameSize);
            cursor += nameSize;

            //GLOG("Reading node name: %s, size = %u", nodeName.c_str(), nameSize);

            uint32_t animType;
            memcpy(&animType, cursor, sizeof(uint32_t));
            cursor += sizeof(uint32_t);

            uint32_t keyframeCount;
            memcpy(&keyframeCount, cursor, sizeof(uint32_t));
            cursor                    += sizeof(uint32_t);


            animChannel.numPositions   = keyframeCount;
            animChannel.posTimeStamps  = std::make_unique<float[]>(keyframeCount);
            memcpy(animChannel.posTimeStamps.get(), cursor, keyframeCount * sizeof(float));
            cursor                += keyframeCount * sizeof(float);


            if(animType == 0) // Translation (float3)
            {
                animChannel.positions = std::make_unique<float3[]>(keyframeCount);
                memcpy(animChannel.positions.get(), cursor, keyframeCount * sizeof(float3));
                cursor += keyframeCount * sizeof(float3);
                if (animChannel.numPositions > 0)
                {
                    for (uint32_t i = 0; i < animChannel.numPositions; ++i)
                    {
                        //GLOG("Keyframe %u: Time = %f, Position = (%f, %f, %f)", i, animChannel.posTimeStamps[i], animChannel.positions[i].x, animChannel.positions[i].y, animChannel.positions[i].z);
                    }
                }
                else
                {
                    GLOG("No keyframes for this animation channel: %s", nodeName.c_str());
                }
            }
            else if (animType == 1) // Rotation (Quat)
            {
                animChannel.rotations = std::make_unique<Quat[]>(keyframeCount);
                memcpy(animChannel.rotations.get(), cursor, keyframeCount * sizeof(Quat));
                cursor += keyframeCount * sizeof(Quat);
            }

            animation->channels[nodeName]  = std::move(animChannel);
        }

        delete[] buffer;

        return animation;
    }
}