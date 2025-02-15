#pragma once

#include "Module.h"

#include "SDL.h"
#include <deque>

struct CPUFeature
{
    SDL_bool (*check)();
    const char *name;
};

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
    void LimitFPS(float deltaTime) const;
    void AddFramePlotData(float deltaTime);
    void Draw();

    void MainMenu();
    void EditorSettings(bool &editorSettingsMenu);

    void FramePlots(bool &vsync);
    void WindowConfig(bool &vsync) const;
    void CameraConfig() const;
    void OpenGLConfig() const;
    void GameTimerConfig() const;
    void HardwareConfig() const;
    void ShowCaps() const;

    void Console(bool &consoleMenu) const;
    void About(bool &aboutMenu) const;

  private:
    bool consoleMenu        = false;
    bool aboutMenu          = false;
    bool editorSettingsMenu = false;
    bool closeApplication   = false;

    int maxFPS              = 60;
    int maximumPlotData     = 50;
    std::deque<float> framerate;
    std::deque<float> frametime;

    EditorViewport *editorViewport = nullptr;
};
