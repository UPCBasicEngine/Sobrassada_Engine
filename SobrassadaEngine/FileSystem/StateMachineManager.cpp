#include "StateMachineManager.h"

#include "Application.h"
#include "FileSystem.h"
#include "LibraryModule.h"
#include "MetaStateMachine.h"
#include "ProjectModule.h"
#include "ResourceStateMachine.h"

#include "Math/Quat.h"
#include "Math/float4x4.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <filesystem>
#include <queue>

namespace StateMachineManager
{
    UID SaveStateMachine(const ResourceStateMachine* resource, bool override)
    {
        // Create doc Json
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        rapidjson::Value stateMachineJSON(rapidjson::kObjectType);

        const UID uid = GenerateUID();
        const UID stateMachineUID =
            override ? resource->GetUID() : App->GetLibraryModule()->AssignFiletypeUID(uid, FileType::StateMachine);
        const std::string savePath = App->GetProjectModule()->GetLoadedProjectPath() + STATEMACHINES_LIB_PATH +
                                     std::to_string(stateMachineUID) + STATEMACHINE_EXTENSION;
        const std::string stateName = resource->GetName();

        stateMachineJSON.AddMember("UID", stateMachineUID, allocator);
        stateMachineJSON.AddMember("Name", rapidjson::Value(stateName.c_str(), allocator), allocator);
        rapidjson::Value triggersArray(rapidjson::kArrayType);

        for (const std::string& trigger : resource->availableTriggers)
        {
            rapidjson::Value triggerValue;
            triggerValue.SetString(trigger.c_str(), static_cast<rapidjson::SizeType>(trigger.length()), allocator);
            triggersArray.PushBack(triggerValue, allocator);
        }

        stateMachineJSON.AddMember("Triggers", triggersArray, allocator);

        // Clips
        rapidjson::Value clipsArray(rapidjson::kArrayType);
        for (const auto& clip : resource->clips)
        {
            rapidjson::Value clipJSON(rapidjson::kObjectType);
            clipJSON.AddMember("ClipUID", clip.animationResourceUID, allocator);
            clipJSON.AddMember("ClipName", rapidjson::Value(clip.clipName.c_str(), allocator), allocator);
            clipJSON.AddMember("ClipTime", clip.animationSpeed, allocator);
            clipJSON.AddMember("Loop", clip.loop, allocator);

            clipsArray.PushBack(clipJSON, allocator);
        }
        stateMachineJSON.AddMember("Clips", clipsArray, allocator);

        // States
        rapidjson::Value statesArray(rapidjson::kArrayType);
        for (const auto& state : resource->states)
        {
            rapidjson::Value stateJSON(rapidjson::kObjectType);
            stateJSON.AddMember("StateName", rapidjson::Value(state.name.c_str(), allocator), allocator);
            stateJSON.AddMember("ClipName", rapidjson::Value(state.clipName.c_str(), allocator), allocator);
            rapidjson::Value position(rapidjson::kObjectType);
            position.AddMember("x", state.position.x, allocator);
            position.AddMember("y", state.position.y, allocator);
            stateJSON.AddMember("Position", position, allocator);

            statesArray.PushBack(stateJSON, allocator);
        }
        stateMachineJSON.AddMember("States", statesArray, allocator);

        // Transitions
        rapidjson::Value transitionsArray(rapidjson::kArrayType);
        for (const auto& transition : resource->transitions)
        {
            rapidjson::Value transitionJSON(rapidjson::kObjectType);
            transitionJSON.AddMember(
                "FromState", rapidjson::Value(transition.fromState.c_str(), allocator), allocator
            );
            transitionJSON.AddMember(
                "ToState", rapidjson::Value(transition.toState.c_str(), allocator), allocator
            );
            transitionJSON.AddMember(
                "Trigger", rapidjson::Value(transition.triggerName.c_str(), allocator), allocator
            );
            transitionJSON.AddMember("InterpolationTime", transition.interpolationTime, allocator);

            transitionsArray.PushBack(transitionJSON, allocator);
        }
        stateMachineJSON.AddMember("Transitions", transitionsArray, allocator);

        doc.AddMember("StateMachine", stateMachineJSON, allocator);
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        // Create meta file
        std::string assetPath = STATEMACHINES_ASSETS_PATH + stateName + STATEMACHINE_EXTENSION;
        MetaStateMachine meta(stateMachineUID, assetPath);
        meta.Save(stateName, assetPath);
        assetPath = App->GetProjectModule()->GetLoadedProjectPath() + STATEMACHINES_ASSETS_PATH + stateName +
                    STATEMACHINE_EXTENSION;

        unsigned int bytesWritten = (unsigned int
        )FileSystem::Save(assetPath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);
        if (bytesWritten == 0)
        {
            GLOG("Failed to save stateMachine file: %s", assetPath.c_str());
            return INVALID_UID;
        }

        bytesWritten =
            (unsigned int)FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);

        if (bytesWritten == 0)
        {
            GLOG("Failed to save state machine file: %s", savePath.c_str());
            return INVALID_UID;
        }
        //GLOG("Saved in %s", savePath.c_str());
        App->GetLibraryModule()->AddStateMachine(stateMachineUID, stateName);
        App->GetLibraryModule()->AddName(stateName, stateMachineUID);
        App->GetLibraryModule()->AddResource(savePath, stateMachineUID);

        return stateMachineUID;
    }

    void CopyMachine(
        const std::string& filePath, const std::string& targetFilePath, const std::string& name, const UID sourceUID
    )
    {
        const std::string destination =
            targetFilePath + STATEMACHINES_LIB_PATH + std::to_string(sourceUID) + STATEMACHINE_EXTENSION;
        FileSystem::Copy(filePath.c_str(), destination.c_str());

        App->GetLibraryModule()->AddStateMachine(sourceUID, name);
        App->GetLibraryModule()->AddName(name, sourceUID);
        App->GetLibraryModule()->AddResource(destination, sourceUID);
    }

    ResourceStateMachine* LoadStateMachine(UID stateMachineUID)
    {
        std::string path = App->GetLibraryModule()->GetResourcePath(stateMachineUID);
        if (path.empty())
        {
            GLOG("Invalid path for StateMachine UID %llu", stateMachineUID);
            return nullptr;
        }

        rapidjson::Document doc;
        bool loaded = FileSystem::LoadJSON(path.c_str(), doc);
        if (!loaded || !doc.HasMember("StateMachine"))
        {
            GLOG("Failed to parse StateMachine JSON: %s", path.c_str());
            return nullptr;
        }

        const rapidjson::Value& stateMachineJSON = doc["StateMachine"];

        UID uid                                  = stateMachineJSON["UID"].GetUint64();
        std::string name                         = stateMachineJSON["Name"].GetString();

        ResourceStateMachine* stateMachine       = new ResourceStateMachine(uid, name);

        if (stateMachineJSON.HasMember("Triggers") && stateMachineJSON["Triggers"].IsArray())
        {
            stateMachine->availableTriggers.clear();
            const rapidjson::Value& triggersArray = stateMachineJSON["Triggers"];

            for (rapidjson::SizeType i = 0; i < triggersArray.Size(); ++i)
            {
                if (triggersArray[i].IsString())
                {
                    stateMachine->availableTriggers.push_back(triggersArray[i].GetString());
                }
            }
        }

        // Clips
        if (stateMachineJSON.HasMember("Clips") && stateMachineJSON["Clips"].IsArray())
        {
            for (const auto& clipJSON : stateMachineJSON["Clips"].GetArray())
            {
                Clip clip;
                clip.animationResourceUID = clipJSON["ClipUID"].GetUint64();
                clip.clipName             = HashString(clipJSON["ClipName"].GetString());
                clip.animationSpeed       = clipJSON.HasMember("ClipTime") ? clipJSON["ClipTime"].GetFloat() : 1.f;
                clip.loop                 = clipJSON["Loop"].GetBool();

                stateMachine->clips.push_back(clip);
                stateMachine->clipsDefaultSpeed.push_back(clip.animationSpeed);
            }
        }

        // States
        if (stateMachineJSON.HasMember("States") && stateMachineJSON["States"].IsArray())
        {
            for (const auto& stateJSON : stateMachineJSON["States"].GetArray())
            {
                State state;
                state.name     = HashString(stateJSON["StateName"].GetString());
                state.clipName = HashString(stateJSON["ClipName"].GetString());
                if (stateJSON.HasMember("Position") && stateJSON["Position"].IsObject())
                {
                    state.position.x = stateJSON["Position"]["x"].GetFloat();
                    state.position.y = stateJSON["Position"]["y"].GetFloat();
                }

                stateMachine->states.push_back(state);
            }
        }

        // Transitions
        if (stateMachineJSON.HasMember("Transitions") && stateMachineJSON["Transitions"].IsArray())
        {
            for (const auto& transitionJSON : stateMachineJSON["Transitions"].GetArray())
            {
                Transition t;
                t.fromState         = HashString(transitionJSON["FromState"].GetString());
                t.toState           = HashString(transitionJSON["ToState"].GetString());
                t.triggerName       = HashString(transitionJSON["Trigger"].GetString());
                t.interpolationTime = transitionJSON["InterpolationTime"].GetInt();

                stateMachine->transitions.push_back(t);
            }
        }

        stateMachine->SetDefaultState(0);
        //GLOG("StateMachine %llu loaded successfully from: %s", stateMachineUID, path.c_str());

        return stateMachine;
    }

} // namespace StateMachineManager

std::vector<std::string> StateMachineManager::GetAllStateMachineNames()
{
    std::vector<std::string> names;
    const auto& stateMachineMap = App->GetLibraryModule()->GetStateMachineMap();

    for (const auto& pair : stateMachineMap)
    {
        names.push_back(pair.first.GetString());
    }

    return names;
}

UID StateMachineManager::GetStateMachineUID(const std::string& stateMachineName)
{
    const auto& stateMachineMap = App->GetLibraryModule()->GetStateMachineMap();

    auto it                     = stateMachineMap.find(stateMachineName);
    if (it != stateMachineMap.end())
    {
        return it->second;
    }

    return INVALID_UID;
} // namespace StateMachineManager
