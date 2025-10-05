#pragma once
#include "Script.h"
#include <string>
#include <vector>

class GameObject;
class PauseMenuScript;
class GameOverScript;
class AudioSourceComponent;

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
    // Inspector-configurable
    std::string panelName = "PauseMenuPanel";

    // Cached panel & UI elements
    GameObject* panelRoot = nullptr;
    std::vector<GameObject*> menuItems;
    std::vector<GameObject*> arrowImages;

    // Navigation state
    int selectedIndex            = 0;
    bool stickMoved              = false;
    bool builtOnce               = false;

    // Controllers (used to close Pause/GameOver)
    PauseMenuScript* pauseCtrl   = nullptr;
    GameOverScript* gameOverCtrl = nullptr;

    // Helpers
    void CachePanel();
    void BuildFromPanel();
    void UpdateSelection();

    AudioSourceComponent* audio = nullptr;
};
