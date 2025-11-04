#pragma once

#include "Module.h"

class GameDebugUIModule : public Module
{
  public:
    GameDebugUIModule();
    ~GameDebugUIModule() override;

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
    update_status RenderEditor(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

  private:
    void Draw();

    void GameDebugMenu();
    void RenderOptions();

  private:
    bool gameDebugMenu    = false;
    bool closeApplication = false;
    bool loadMenu = false;

    bool drawFPS = false;
    bool openConsole = false;
};
