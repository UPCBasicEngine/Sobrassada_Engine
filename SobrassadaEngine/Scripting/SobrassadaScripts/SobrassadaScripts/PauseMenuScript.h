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

    void Show();  // open & pause
    void Close(); // close & unpause
    void Toggle();

  private:
    void CachePanel();

    std::string panelToShowName = "PauseMenuPanel";
    GameObject* cachedTarget    = nullptr;
    bool isOpen                 = false;
};
