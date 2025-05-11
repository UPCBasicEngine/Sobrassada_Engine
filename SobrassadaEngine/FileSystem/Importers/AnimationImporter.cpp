#include "AnimationImporter.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "MetaAnimation.h"
#include "ProjectModule.h"
#include "ResourceAnimation.h"

#include "rapidjson/document.h"
#include <memory>
#include <tiny_gltf.h>
#include <vector>

namespace AnimationImporter
{
    UID ImportAnimation(
        const tinygltf::Model& model, const tinygltf::Animation& animation, const std::string& name,
        const char* sourceFilePath, const std::string& targetFilePath, UID sourceUID, int animationIndex
    )
    {
        std::vector<char> buffer;

        uint32_t channelCount = static_cast<uint32_t>(animation.channels.size());
        buffer.insert(
            buffer.end(), reinterpret_cast<char*>(&channelCount),
            reinterpret_cast<char*>(&channelCount) + sizeof(uint32_t)
        );

        GLOG("Importing animation '%s' with %d channels", animation.name.c_str(), channelCount);

        for (const auto& channel : animation.channels)
        {
            const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];

            const tinygltf::Accessor& input           = model.accessors[sampler.input];
            const tinygltf::Accessor& output          = model.accessors[sampler.output];

            const tinygltf::BufferView& inputView     = model.bufferViews[input.bufferView];
            const tinygltf::BufferView& outputView    = model.bufferViews[output.bufferView];

            const tinygltf::Buffer& inputBuffer       = model.buffers[inputView.buffer];
            const tinygltf::Buffer& outputBuffer      = model.buffers[outputView.buffer];

            // Check if buffer access is valid
            if (input.byteOffset + inputView.byteOffset >= inputBuffer.data.size() ||
                output.byteOffset + outputView.byteOffset >= outputBuffer.data.size())
            {
                // GLOG("Error: Invalid buffer access for animation channel");
                continue; // Skip this channel
            }

            // Handle stride properly for (timestamps)
            size_t inputStride             = (inputView.byteStride > 0) ? inputView.byteStride : sizeof(float);
            const unsigned char* inputData = &inputBuffer.data[input.byteOffset + inputView.byteOffset];

            std::vector<float> timeStamps(input.count);
            for (size_t i = 0; i < input.count; i++)
            {
                timeStamps[i] = *reinterpret_cast<const float*>(inputData + i * inputStride);
            }

            const size_t keyframeCount  = input.count;

            const std::string& nodeName = model.nodes[channel.target_node].name;
            const uint32_t nameSize     = static_cast<uint32_t>(nodeName.size());

            /*
            GLOG(
                "Channel: Node='%s', Path='%s', Keyframes=%zu", nodeName.c_str(), channel.target_path.c_str(),
                keyframeCount
            );*/

            buffer.insert(
                buffer.end(), reinterpret_cast<const char*>(&nameSize),
                reinterpret_cast<const char*>(&nameSize) + sizeof(uint32_t)
            );
            buffer.insert(buffer.end(), nodeName.begin(), nodeName.end());

            AnimationType animType;
            if (channel.target_path == "rotation")
            {
                animType = AnimationType::ROTATION;
            }
            else if (channel.target_path == "translation")
            {
                animType = AnimationType::TRANSLATION;
            }
            else if (channel.target_path == "scale")
            {
                animType = AnimationType::SCALE;
            }
            else
            {
                GLOG("Skipping unsupported animation path: %s", channel.target_path.c_str());
                continue;
            }

            buffer.insert(
                buffer.end(), reinterpret_cast<const char*>(&animType),
                reinterpret_cast<const char*>(&animType) + sizeof(AnimationType)
            );

            buffer.insert(
                buffer.end(), reinterpret_cast<const char*>(&keyframeCount),
                reinterpret_cast<const char*>(&keyframeCount) + sizeof(uint32_t)
            );

            // Insert timestamps
            if (keyframeCount > 0)
            {
                //GLOG("  Timestamps range: %.3f to %.3f", timeStamps[0], timeStamps[keyframeCount - 1]);

                buffer.insert(
                    buffer.end(), reinterpret_cast<const char*>(timeStamps.data()),
                    reinterpret_cast<const char*>(timeStamps.data()) + keyframeCount * sizeof(float)
                );
            }

            // Handle stride properly for output values
            const unsigned char* outputData = &outputBuffer.data[output.byteOffset + outputView.byteOffset];
            size_t outputStride;

            if (animType == AnimationType::TRANSLATION)
            {
                outputStride = (outputView.byteStride > 0) ? outputView.byteStride : sizeof(float3);

                // Copy with proper stride
                std::vector<float3> positionValues(keyframeCount);
                for (size_t i = 0; i < keyframeCount; i++)
                {
                    const float3* srcPos = reinterpret_cast<const float3*>(outputData + i * outputStride);
                    positionValues[i]    = *srcPos;
                }

                // Debug output
                /* GLOG("  Position data for node %s:", nodeName.c_str());
                 for (size_t i = 0; i < std::min(keyframeCount, size_t(3)); i++)
                 {
                     GLOG(
                         "    Frame %zu: (%.3f, %.3f, %.3f) at time %.3f", i, positionValues[i].x, positionValues[i].y,
                         positionValues[i].z, timeStamps[i]
                     );
                 }*/

                buffer.insert(
                    buffer.end(), reinterpret_cast<const char*>(positionValues.data()),
                    reinterpret_cast<const char*>(positionValues.data()) + keyframeCount * sizeof(float3)
                );
            }
            else if (animType == AnimationType::ROTATION)
            {
                outputStride = (outputView.byteStride > 0) ? outputView.byteStride : sizeof(Quat);

                std::vector<Quat> rotationValues(keyframeCount);
                for (size_t i = 0; i < keyframeCount; i++)
                {
                    const Quat* srcRot = reinterpret_cast<const Quat*>(outputData + i * outputStride);
                    rotationValues[i]  = *srcRot;

                    rotationValues[i].Normalize();
                }

                // GLOG("  Rotation data for node %s:", nodeName.c_str());
                /*for (size_t i = 0; i < std::min(keyframeCount, size_t(3)); i++)
                {
                    GLOG(
                        "    Frame %zu: (%.3f, %.3f, %.3f, %.3f) at time %.3f", i, rotationValues[i].x,
                        rotationValues[i].y, rotationValues[i].z, rotationValues[i].w, timeStamps[i]
                    );
                }*/

                buffer.insert(
                    buffer.end(), reinterpret_cast<const char*>(rotationValues.data()),
                    reinterpret_cast<const char*>(rotationValues.data()) + keyframeCount * sizeof(Quat)
                );
            }
            else if (animType == AnimationType::SCALE)
            {
                outputStride = (outputView.byteStride > 0) ? outputView.byteStride : sizeof(float3);

                std::vector<float3> scaleValues(keyframeCount);
                for (size_t i = 0; i < keyframeCount; i++)
                {
                    const float3* srcScale = reinterpret_cast<const float3*>(outputData + i * outputStride);
                    scaleValues[i]         = *srcScale;
                }

                /*GLOG("  Scale data for node %s:", nodeName.c_str());
                for (size_t i = 0; i < std::min(keyframeCount, size_t(3)); i++)
                {
                    GLOG(
                        "    Frame %zu: (%.3f, %.3f, %.3f) at time %.3f", i, scaleValues[i].x, scaleValues[i].y,
                        scaleValues[i].z, timeStamps[i]
                    );
                }*/

                buffer.insert(
                    buffer.end(), reinterpret_cast<const char*>(scaleValues.data()),
                    reinterpret_cast<const char*>(scaleValues.data()) + keyframeCount * sizeof(float3)
                );
            }
        }

        // Handle UID
        const std::string fileName = FileSystem::GetFileNameWithoutExtension(sourceFilePath) + "_" + animation.name;
        UID finalAnimUID;
        if (sourceUID == INVALID_UID)
        {
            UID animationUID           = GenerateUID();
            animationUID               = App->GetLibraryModule()->AssignFiletypeUID(animationUID, FileType::Animation);

            UID prefix                 = animationUID / UID_PREFIX_DIVISOR;
            const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + METADATA_PATH +
                                         std::to_string(prefix) + FILENAME_SEPARATOR + fileName + META_EXTENSION;
            finalAnimUID = App->GetLibraryModule()->GetUIDFromMetaFile(savePath);
            if (finalAnimUID == INVALID_UID) finalAnimUID = animationUID;

            const std::string assetPath = ASSETS_PATH + FileSystem::GetFileNameWithExtension(sourceFilePath);
            MetaAnimation meta(finalAnimUID, assetPath, animationIndex);

            meta.Save(fileName, assetPath);
        }
        else
        {
            finalAnimUID = sourceUID;
        }

        // Construct save paths
        const std::string saveFilePath = App->GetProjectModule()->GetLoadedProjectPath() + ANIMATIONS_PATH +
                                         std::to_string(finalAnimUID) + ANIMATION_EXTENSION;

        //GLOG("Saving animation to: %s", saveFilePath.c_str());

        const size_t bytesWritten =
            FileSystem::Save(saveFilePath.c_str(), buffer.data(), static_cast<unsigned int>(buffer.size()), true);

        if (bytesWritten == 0)
        {
            GLOG("Failed to save animation file: %s", saveFilePath.c_str());
            return 0;
        }

        App->GetLibraryModule()->AddAnimation(finalAnimUID, fileName);
        App->GetLibraryModule()->AddName(fileName, finalAnimUID);
        App->GetLibraryModule()->AddResource(saveFilePath, finalAnimUID);

        //GLOG("%s saved as binary (%zu bytes written)", fileName.c_str(), bytesWritten);

        return finalAnimUID;
    }

    ResourceAnimation* LoadAnimation(UID animationUID)
    {
        const std::string path = App->GetLibraryModule()->GetResourcePath(animationUID);
        //GLOG("Attempting to load animation from: %s", path.c_str());

        char* buffer                = nullptr;
        const unsigned int fileSize = FileSystem::Load(path.c_str(), &buffer);

        //GLOG("Load result: fileSize=%u, buffer=%p", fileSize, buffer);

        if (fileSize == 0 || buffer == nullptr)
        {
            GLOG("Failed to load animation file: %s", path.c_str());
            return nullptr;
        }

        char* cursor    = buffer;
        char* bufferEnd = buffer + fileSize;

        // Ensure  enough space to read the channel count
        if (cursor + sizeof(uint32_t) > bufferEnd)
        {
            GLOG("Error: File too small to contain channel count");
            delete[] buffer;
            return nullptr;
        }

        uint32_t channelCount;
        memcpy(&channelCount, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);

        //GLOG("Loading animation with %d channels from %s", channelCount, path.c_str());

        const auto& map = App->GetLibraryModule()->GetAnimMap();
        const std::string& animName    = App->GetLibraryModule()->GetResourceName(animationUID);

        ResourceAnimation* animation = new ResourceAnimation(animationUID, animName);

        // Parse channels
        for (uint32_t i = 0; i < channelCount; ++i)
        {
            // Check if enough buffer left for nameSize
            if (cursor + sizeof(uint32_t) > bufferEnd)
            {
                GLOG("Error: Unexpected end of file when reading channel %d nameSize", i);
                break;
            }

            uint32_t nameSize;
            memcpy(&nameSize, cursor, sizeof(uint32_t));
            cursor += sizeof(uint32_t);

            // Check if enough buffer left for the name
            if (cursor + nameSize > bufferEnd)
            {
                GLOG("Error: Unexpected end of file when reading channel %d name", i);
                break;
            }

            std::string nodeName(cursor, nameSize);
            cursor += nameSize;

            if (cursor + sizeof(AnimationType) > bufferEnd)
            {
                GLOG("Error: Unexpected end of file when reading channel %d animType", i);
                break;
            }

            AnimationType animType;
            memcpy(&animType, cursor, sizeof(AnimationType));
            cursor += sizeof(AnimationType);

            if (cursor + sizeof(uint32_t) > bufferEnd)
            {
                GLOG("Error: Unexpected end of file when reading channel %d keyframeCount", i);
                break;
            }

            uint32_t keyframeCount;
            memcpy(&keyframeCount, cursor, sizeof(uint32_t));
            cursor               += sizeof(uint32_t);

            // GLOG(
            //     "Channel %d: Node='%s', Type=%d, Keyframes=%d", i, nodeName.c_str(), static_cast<int>(animType),
            //     keyframeCount
            //);

            Channel& animChannel  = animation->channels[nodeName];

            // Parse based on animation type
            if (animType == AnimationType::TRANSLATION)
            {
                uint32_t posTimestampSize = keyframeCount * sizeof(float);
                uint32_t posDataSize      = keyframeCount * sizeof(float3);

                // Validate we have enough data left in the buffer
                if (cursor + posTimestampSize + posDataSize <= bufferEnd)
                {
                    animChannel.posTimeStamps.resize(keyframeCount);
                    memcpy(animChannel.posTimeStamps.data(), cursor, posTimestampSize);
                    cursor += posTimestampSize;

                    animChannel.positions.resize(keyframeCount);
                    memcpy(animChannel.positions.data(), cursor, posDataSize);

                    // Debug log the loaded positions
                    /* GLOG("  Loaded %d position keyframes for '%s'", keyframeCount, nodeName.c_str());
                     for (int j = 0; j < std::min(3, (int)keyframeCount); j++)
                     {
                         GLOG(
                             "    Frame %d: (%.3f, %.3f, %.3f) at time %.3f", j, animChannel.positions[j].x,
                             animChannel.positions[j].y, animChannel.positions[j].z, animChannel.posTimeStamps[j]
                         );
                     }*/

                    cursor                   += posDataSize;
                    animChannel.numPositions  = keyframeCount;
                }
                else
                {
                    // GLOG("Error: Buffer overrun when reading position data for channel %d", i);
                    break; // Stop parsing to avoid further issues
                }
            }
            else if (animType == AnimationType::ROTATION)
            {
                uint32_t rotTimestampSize = keyframeCount * sizeof(float);
                uint32_t rotDataSize      = keyframeCount * sizeof(Quat);

                if (cursor + rotTimestampSize + rotDataSize <= bufferEnd)
                {
                    animChannel.rotTimeStamps.resize(keyframeCount);
                    memcpy(animChannel.rotTimeStamps.data(), cursor, rotTimestampSize);
                    cursor += rotTimestampSize;

                    animChannel.rotations.resize(keyframeCount);
                    memcpy(animChannel.rotations.data(), cursor, rotDataSize);

                    /* GLOG("  Loaded %d rotation keyframes for '%s'", keyframeCount, nodeName.c_str());
                     for (int j = 0; j < std::min(3, (int)keyframeCount); j++)
                     {
                         GLOG(
                             "    Frame %d: (%.3f, %.3f, %.3f, %.3f) at time %.3f", j, animChannel.rotations[j].x,
                             animChannel.rotations[j].y, animChannel.rotations[j].z, animChannel.rotations[j].w,
                             animChannel.rotTimeStamps[j]
                         );
                     }*/

                    cursor                   += rotDataSize;
                    animChannel.numRotations  = keyframeCount;
                }
                else
                {
                    // GLOG("Error: Buffer overrun when reading rotation data for channel %d", i);
                    break; // Stop parsing to avoid further issues
                }
            }
            else if (animType == AnimationType::SCALE)
            {
                uint32_t scaleTimestampSize = keyframeCount * sizeof(float);
                uint32_t scaleDataSize      = keyframeCount * sizeof(float3);

                if (cursor + scaleTimestampSize + scaleDataSize <= bufferEnd)
                {
                    animChannel.scaleTimeStamps.resize(keyframeCount);
                    memcpy(animChannel.scaleTimeStamps.data(), cursor, scaleTimestampSize);
                    cursor += scaleTimestampSize;

                    animChannel.scales.resize(keyframeCount);
                    memcpy(animChannel.scales.data(), cursor, scaleDataSize);

                    // GLOG("  Loaded %d scale keyframes for '%s'", keyframeCount, nodeName.c_str());
                    for (int j = 0; j < std::min(3, (int)keyframeCount); j++)
                    {
                        /*GLOG(
                            "    Frame %d: (%.3f, %.3f, %.3f) at time %.3f", j, animChannel.scales[j].x,
                            animChannel.scales[j].y, animChannel.scales[j].z, animChannel.scaleTimeStamps[j]
                        );*/
                    }

                    cursor                += scaleDataSize;
                    animChannel.numScales  = keyframeCount;
                }
                else
                {
                    // GLOG("Error: Buffer overrun when reading scale data for channel %d", i);
                    break; // Stop parsing to avoid further issues
                }
            }
        }

        delete[] buffer;

        animation->SetDuration();

        //GLOG("Animation duration: %f", animation->GetDuration());

        return animation;
    }
} // namespace AnimationImporter