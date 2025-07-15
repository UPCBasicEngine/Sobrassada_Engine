#include "WindConfig.h"

void WindConfig::SaveData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    targetState.AddMember("Apply wind globally", applyWindGlobally, allocator);
    targetState.AddMember("Wind direction", windDirection, allocator);
    targetState.AddMember("Wind speed", windSpeed, allocator);
    targetState.AddMember("Gust frequency", gustFrequency, allocator);
    targetState.AddMember("Gust speed", gustSpeed, allocator);
}

void WindConfig::LoadData(const rapidjson::Value& wind)
{
    applyWindGlobally = wind["Apply wind globally"].GetBool();
    windDirection = wind["Wind direction"].GetFloat();
    windSpeed = wind["Wind speed"].GetFloat();
    gustFrequency = wind["Gust frequency"].GetFloat();
    gustSpeed = wind["Gust speed"].GetFloat();
}

