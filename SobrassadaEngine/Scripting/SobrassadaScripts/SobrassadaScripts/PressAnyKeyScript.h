#pragma once

#include "Script.h"
#include <string>

class PressAnyKeyScript : public Script
{
  public:

    PressAnyKeyScript(GameObject* parent);
    bool Init() override;
    void Update(float deltaTime) override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) override;
    void Load(const rapidjson::Value& initialState) override;


  private:
    std::string nextGameObjectName;
};
