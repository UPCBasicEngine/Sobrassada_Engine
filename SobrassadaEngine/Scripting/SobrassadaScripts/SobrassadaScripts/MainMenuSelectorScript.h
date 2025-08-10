#pragma once
#include "Script.h"
#include <vector>

class PauseMenuScript;
class GameOverScript;

class MainMenuSelectorScript : public Script
{
  public:
    MainMenuSelectorScript(GameObject* parent) : Script(parent) {}

    bool Init() override;
    void Update(float deltaTime) override;
    void Inspector() override {}

  private:
    std::vector<GameObject*> menuItems;
    std::vector<GameObject*> arrowImages;
    int selectedIndex            = 0;
    bool stickMoved              = false;

    PauseMenuScript* pauseCtrl   = nullptr;
    GameOverScript* gameOverCtrl = nullptr;

    void UpdateSelection();
};
