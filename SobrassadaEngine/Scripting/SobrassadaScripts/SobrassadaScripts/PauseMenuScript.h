#pragma once

#include "Script.h"
#include <string>

class PauseMenuScript : public Script
{
  public:
    PauseMenuScript(GameObject* parent) : Script(parent) {}

    bool Init() override;
    void Update(float deltaTime) override;
    void Inspector() override {};
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator);
    void Load(const rapidjson::Value& initialState) override;

  private:
    std::string panelToShowName = "PauseMenuPanel";
    GameObject* cachedTarget    = nullptr;
};
