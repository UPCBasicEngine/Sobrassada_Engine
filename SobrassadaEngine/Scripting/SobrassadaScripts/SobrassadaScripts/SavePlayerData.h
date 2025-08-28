#pragma once
#include "rapidjson/document.h"
#include <string>

struct PlayerState
{
    int currentHealth     = 0;
    int maxHealth         = 0;
    int riastrad          = 0;
    bool dashUnlocked     = false;
    bool ultimateUnlocked = false;
};

namespace SavePlayerData
{
    void Save(rapidjson::Value& targetState,rapidjson::Document::AllocatorType& allocator, const PlayerState& playerState);
    void Load(const rapidjson::Value& source, PlayerState& playerState);

    bool EnsureParentDir(const std::string& filePath);
    std::string MakeSavePath(const std::string& projectPath) const{ return projectPath + "/Saves/player_state.json"; }
    bool SavePlayerToFile(const PlayerState& playerState, const std::string& filePath);
    bool LoadPlayerFromFile(PlayerState& playerState, const std::string& filePath);
} // namespace SavePlayerData