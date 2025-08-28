#include "SavePlayerData.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "rapidjson/writer.h"
#include "rapidjson/ostreamwrapper.h"


namespace SavePlayerData
{

    void Save(
        rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator, const PlayerState& playerState
    )
    {
        doc.AddMember("currentHealth", playerState.currentHealth, allocator);
        doc.AddMember("maxHealth", playerState.maxHealth, allocator);
        doc.AddMember("riastrad", playerState.riastrad, allocator);
        doc.AddMember("dashUnlocked", playerState.dashUnlocked, allocator);
        doc.AddMember("ultimateUnlocked", playerState.ultimateUnlocked, allocator);
    }

    void Load(const rapidjson::Value& source, PlayerState& playerState)
    {
        if (source.HasMember("currentHealth") && source.["currentHealth"].IsInt())
            playerState.currentHealth = source["currentHealth"].GetInt();
        if (source.HasMember("maxHealth") && source.["maxHealth"].IsInt())
            playerState.currentHealth = source["maxHealth"].GetInt();
        if (source.HasMember("riastrad") && source.["riastrad"].IsInt())
            playerState.currentHealth = source["riastrad"].GetInt();
        if (source.HasMember("dashUnlocked") && source.["dashUnlocked"].IsBool())
            playerState.currentHealth = source["dashUnlocked"].GetBool();
        if (source.HasMember("ultimateUnlocked") && source.["ultimateUnlocked"].IsBool())
            playerState.currentHealth = source["ultimateUnlocked"].GetBool();
    }

    bool EnsureParentDir(const std::string& filePath)
    {
        std::filesystem::path p(filePath);
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec); //change path later to the good one
        return !ec;
    }

    bool SavePlayerToFile(const PlayerState& playerState, const std::string& filePath)
    {
        if (!EnsureParentDir(filePath)) return false;

        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        rapidjson::Value player(rapidjson::kObjectType);
        Save(player, alloc, playerState);
        doc.AddMember("Player", player, alloc);

        std::ofstream ofs(filePath,std::ios::trunc);
        if (!ofs) return false;

        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        doc.Accept(writer);

        ofs << buffer.GetString();
        return true;
    }

    bool LoadPlayerFromFile(PlayerState& playerState, const std::string& filePath)
    {
        std::ifstream ifs(filePath);
        if (!ifs) return false;

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string json = ss.str();

        Document doc;
        doc.Parse(json.c_str());
        if (doc.HasParseError() || !doc.IsObject()) return false;

        if (!doc.HasMember["Player"] || !doc["Player"].IsObject()) return false;

        Load(doc.["Player"], playerState);
        return false;
    }

} // namespace SavePlayerData