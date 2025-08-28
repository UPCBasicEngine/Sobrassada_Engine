#include "pch.h"
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
        targetState.AddMember("currentHealth", playerState.currentHealth, allocator);
        targetState.AddMember("maxHealth", playerState.maxHealth, allocator);
        targetState.AddMember("riastrad", playerState.riastrad, allocator);
        targetState.AddMember("dashUnlocked", playerState.dashUnlocked, allocator);
        targetState.AddMember("ultimateUnlocked", playerState.ultimateUnlocked, allocator);
    }

    void Load(const rapidjson::Value& source, PlayerState& playerState)
    {
        if (source.HasMember("currentHealth") && source["currentHealth"].IsInt())
            playerState.currentHealth = source["currentHealth"].GetInt();
        if (source.HasMember("maxHealth") && source["maxHealth"].IsInt())
            playerState.maxHealth = source["maxHealth"].GetInt();
        if (source.HasMember("riastrad") && source["riastrad"].IsInt())
            playerState.riastrad = source["riastrad"].GetInt();
        if (source.HasMember("dashUnlocked") && source["dashUnlocked"].IsBool())
            playerState.dashUnlocked = source["dashUnlocked"].GetBool();
        if (source.HasMember("ultimateUnlocked") && source["ultimateUnlocked"].IsBool())
            playerState.ultimateUnlocked = source["ultimateUnlocked"].GetBool();
    }

    bool EnsureParentDir(const std::string& filePath)
    {
        std::filesystem::path p(filePath);
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec); //change path later to the good one
        return !ec;
    }

    std::string MakeSavePath(const std::string& projectPath)
    {
        std::filesystem::path base(projectPath);
        std::filesystem::path p = base / "Saves" / "player_state.json";
        return p.string();
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

        return true;
    }

    bool LoadPlayerFromFile(PlayerState& playerState, const std::string& filePath)
    {
        std::ifstream ifs(filePath);
        if (!ifs) return false;

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string json = ss.str();

        rapidjson::Document doc;
        doc.Parse(json.c_str());
        if (doc.HasParseError() || !doc.IsObject()) return false;

        if (!doc.HasMember("Player") || !doc["Player"].IsObject()) return false;

        Load(doc["Player"], playerState);
        return true;
    }

} // namespace SavePlayerData