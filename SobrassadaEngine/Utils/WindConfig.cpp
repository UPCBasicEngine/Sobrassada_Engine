#include "WindConfig.h"

void WindConfig::SaveData(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value windDirectionArray(rapidjson::kArrayType);
    windDirectionArray.PushBack(windDirection.x, allocator)
        .PushBack(windDirection.y, allocator)
        .PushBack(windDirection.z, allocator);

    targetState.AddMember("Apply wind globally", applyWindGlobally, allocator);
    targetState.AddMember("Wind direction", windDirectionArray, allocator);
    targetState.AddMember("Wind speed", windSpeed, allocator);
    targetState.AddMember("Gust frequency", gustFrequency, allocator);
    targetState.AddMember("Gust speed", gustSpeed, allocator);
}

void WindConfig::LoadData(const rapidjson::Value& wind)
{
    const rapidjson::Value& windDirectionArray = wind["Wind direction"];

    applyWindGlobally = wind["Apply wind globally"].GetBool();
    windDirection = {windDirectionArray[0].GetFloat(), windDirectionArray[1].GetFloat(), windDirectionArray[2].GetFloat()};
    windSpeed = wind["Wind speed"].GetFloat();
    gustFrequency = wind["Gust frequency"].GetFloat();
    gustSpeed = wind["Gust speed"].GetFloat();
}

