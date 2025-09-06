#pragma once
#include "Script.h"
#include <string>
#include <vector>

class PauseMenuScript;
class GameOverScript;

class MainMenuSelectorScript : public Script
{
  public:
    MainMenuSelectorScript(GameObject* parent) : Script(parent)
    {
        fields.push_back({"Panel Name", InspectorField::FieldType::InputText, &panelName});
    }

    bool Init() override;
    void Update(float deltaTime) override;
    void Inspector() override {}

  private:
    // Configurable via inspector
    std::string panelName = "PauseMenuPanel";

    // Cached panel + items
    GameObject* panelRoot = nullptr;
    std::vector<GameObject*> menuItems;
    std::vector<GameObject*> arrowImages;

    int selectedIndex            = 0;
    bool stickMoved              = false;
    bool builtOnce               = false;

    PauseMenuScript* pauseCtrl   = nullptr;
    GameOverScript* gameOverCtrl = nullptr;

    // helpers
    void CachePanel_();
    void BuildFromPanel_();
    void UpdateSelection_();
};
