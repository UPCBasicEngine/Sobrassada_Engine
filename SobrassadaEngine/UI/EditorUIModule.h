#pragma once

#include "Module.h"

#include <deque>

class EditorViewport;

class EditorUIModule : public Module
{
  public:
    EditorUIModule();
    ~EditorUIModule();

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
    update_status RenderEditor(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

  private:
    void LimitFPS(float deltaTime);
    void AddFramePlotData(float deltaTime);
    void Draw();

    void MainMenu();
    void EditorSettings(bool &editorSettingsMenu) const;

    void FramePlots(bool &vsync);
    void WindowConfig(bool &vsync);
    void CameraConfig();
    void OpenGLConfig();
    void GameTimerConfig() const;

    void Console(bool &consoleMenu) const;

  private:
    bool consoleMenu        = false;
    bool editorSettingsMenu = false;
    bool closeApplication   = false;

    int maxFPS              = 60;
    int maximumPlotData     = 50;
    std::deque<float> framerate;
    std::deque<float> frametime;

    EditorViewport *editorViewport = nullptr;
};
