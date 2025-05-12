#include "EngineConfig.h"

#include "FileSystem.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

EngineConfig::EngineConfig()
{
    EngineConfig::Load();
}

EngineConfig::~EngineConfig()
{
}

void EngineConfig::SetStartupProjectPath(const std::string& newStartupProjectPath)
{
    startupProjectPath = newStartupProjectPath;
    if (!newStartupProjectPath.empty())
    {
        previouslyLoadedProjectPaths.insert(HashString(startupProjectPath));
    }
    Save();
}

void EngineConfig::Load()
{
    rapidjson::Document doc;
    bool loaded = FileSystem::LoadJSON("Config.json", doc);

    if (!loaded)
    {
        GLOG("Failed to load config file. Generating an empty one. ");
        Save();
        return;
    }

    if (doc.HasMember("StartupProject")) startupProjectPath = doc["StartupProject"].GetString();
    if (doc.HasMember("StartGameOnStartup")) startGameOnStartup = doc["StartGameOnStartup"].GetBool();

    if (doc.HasMember("PreviouslyLoaded") && doc["PreviouslyLoaded"].IsArray())
    {
        previouslyLoadedProjectPaths.clear();
        const rapidjson::Value& previouslyLoaded = doc["PreviouslyLoaded"];

        for (rapidjson::SizeType i = 0; i < previouslyLoaded.Size(); i++)
        {
            previouslyLoadedProjectPaths.insert(HashString(previouslyLoaded[i].GetString()));
        }
    }
}

void EngineConfig::Save() const
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("StartupProject", rapidjson::Value(startupProjectPath.c_str(), allocator), allocator);
    doc.AddMember("StartGameOnStartup", startGameOnStartup, allocator);
    rapidjson::Value valChildren(rapidjson::kArrayType);

    for (const HashString& projectPath : previouslyLoadedProjectPaths)
    {
        valChildren.PushBack(rapidjson::Value(projectPath.c_str(), allocator), allocator);
    }

    doc.AddMember("PreviouslyLoaded", valChildren, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter writer(buffer);
    doc.Accept(writer);

    std::string savePath = "Config.json";
    unsigned int bytesWritten =
        FileSystem::Save(savePath.c_str(), buffer.GetString(), (unsigned int)buffer.GetSize(), false);

    if (bytesWritten == 0) GLOG("Failed to save engine config file: %s", savePath.c_str());
}

void EngineConfig::ClearPreviouslyLoadedProjectPaths()
{
    previouslyLoadedProjectPaths.clear();
    Save();
}