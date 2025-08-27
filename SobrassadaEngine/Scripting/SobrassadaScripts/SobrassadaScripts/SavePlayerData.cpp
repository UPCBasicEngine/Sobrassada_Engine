#include "SavePlayerData.h"


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

} // namespace SavePlayerData