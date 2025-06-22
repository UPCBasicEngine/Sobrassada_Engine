#pragma once
#include "Script.h"
#include <string>
#include <vector>
#include <unordered_map>

class GameObject;

struct TexPair
{
    UID texKeyboard;
    UID texGamepad;
};

class OptionsMenuSwitcherScript : public Script
{
  public:
    
    OptionsMenuSwitcherScript(GameObject* parent) : Script(parent) {}

    bool Init() override;
    void Update(float deltaTime) override;
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) override;
    void Load(const rapidjson::Value& initialState) override;


  private:
    static const std::unordered_map<std::string, TexPair> panelInput;
    static const std::vector<std::string> panelNames;

    bool lastKbState = true;
    int currentIndex = 0;
    bool initialized = false;

    void ApplyDeviceTextures(bool usingKb);
    void ShowOnlyCurrentPanel();
    GameObject* FindPanelByName(const std::string& name) const;
};
