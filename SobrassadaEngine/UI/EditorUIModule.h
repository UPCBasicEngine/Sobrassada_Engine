#pragma once

#include "Module.h"

#include <deque>
#include <string>

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
    void AddFramePlotData(float deltaTime);
    void Draw();

    void MainMenu();
    void EditorSettings(bool &editorSettingsMenu);

    void FramePlots();
    void WindowConfig();
    void CameraConfig();
    void OpenGLConfig();

    void Console(bool &consoleMenu);
    void ImportDialog(bool &import);
    void GetFilesSorted(const std::string &currentPath, std::vector<std::string> &files);
    void LoadDialog(bool &load);
    void SaveDialog(bool &save);

  private:
    bool consoleMenu        = false;
    bool import             = false;
    bool load               = false;
    bool save               = false;
    bool loadScene          = false;
    bool editorSettingsMenu = false;
    bool closeApplication   = false;

    int maximumPlotData     = 50;
    std::deque<float> framerate;
    std::deque<float> frametime;

    EditorViewport *editorViewport = nullptr;

    int width, height;

    std::string startPath;
};