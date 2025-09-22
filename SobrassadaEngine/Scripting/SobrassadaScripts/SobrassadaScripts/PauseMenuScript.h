#pragma once

#include "Script.h"
#include <cstdint>
#include <string>
#include <vector>

class GameObject;

class PauseMenuScript : public Script
{
  public:
    PauseMenuScript(GameObject* parent) : Script(parent) {}

    bool Init() override;
    void Update(float deltaTime) override;
    void Inspector() override {}
    void Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator);
    void Load(const rapidjson::Value& initialState) override;

    void Show();  // open & pause
    void Close(); // close & unpause
    void Toggle();

  private:
    // Cache panel by name
    void CachePanel();

    void BuildFromPanel();
    void DisableAllArrows();
    void UpdateSelection();
    void HandleInput();
    uint64_t GetCurrentTimeMs() const;
    void ReadInputs(bool& upHeld, bool& downHeld, bool& acceptHeld, int& stickDir);

  private:
    std::string panelToShowName = "PauseMenuPanel";

    GameObject* cachedTarget    = nullptr; // panel root
    std::vector<GameObject*> menuItems;
    std::vector<GameObject*> arrowImages;

    bool isOpen       = false;
    bool builtOnce    = false;
    int selectedIndex = 0;

    // input repeat/debounce state
    uint64_t lastMs   = 0;
    uint32_t repeatMs = 0;
    int lastDir       = 0;
    bool acceptWas    = false;

    // previous input edges
    bool upPrev       = false;
    bool downPrev     = false;
    bool accPrev      = false;
};
