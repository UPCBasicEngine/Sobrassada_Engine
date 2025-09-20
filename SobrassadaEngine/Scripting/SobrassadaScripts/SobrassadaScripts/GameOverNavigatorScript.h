#pragma once
#include "Script.h"
#include <cstdint>
#include <string>
#include <vector>

class GameObject;
class GameOverScript;

class GameOverNavigatorScript : public Script
{
  public:
    GameOverNavigatorScript(GameObject* parent) : Script(parent) {}

    bool Init() override;
    void Update(float dt) override;
    void Inspector() override {}

  private:
    // Config (Game Over panel name)
    std::string panelName = "GameOverPanel";

    // Panel state
    GameObject* panelRoot = nullptr;
    std::vector<GameObject*> menuItems;
    std::vector<GameObject*> arrowImages;

    // Reference to GameOverScript (to call Close)
    GameOverScript* goController = nullptr;

    // Navigation state
    int selectedIndex            = 0;
    bool builtOnce               = false;

    // Debounce / autorepeat
    uint64_t lastMs              = 0;
    uint32_t repeatMs            = 0;
    int lastDir                  = 0;
    bool acceptWas               = false;

    // Previous input edges
    bool upPrev                  = false;
    bool downPrev                = false;
    bool accPrev                 = false;
    int stickPrev                = 0;

  private:
    // Helpers
    void CachePanel();
    void BuildFromPanel();
    void UpdateSelection();
    void LocateGameOverScript();
    void ReadInputs(bool& upHeld, bool& downHeld, bool& acceptHeld, int& stickDir);
    uint64_t GetCurrentTimeMs() const;
};
