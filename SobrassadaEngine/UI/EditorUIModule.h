#pragma once

#include "Module.h"

#include "Transform.h"

#include <deque>

class EditorViewport;
class QuadtreeViewer;

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

    bool RenderTransformModifier(Transform &localTransform, Transform &globalTransform, uint32_t uuidParent);

public:
    bool hierarchyMenu      = true;
    bool inspectorMenu      = true; 

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

  private:
    bool consoleMenu            = false;
    bool editorSettingsMenu     = false;
    bool quadtreeViewerViewport = true;
    bool closeApplication       = false;

    int maximumPlotData         = 50;
    std::deque<float> framerate;
    std::deque<float> frametime;

    int transformType = LOCAL;

    EditorViewport *editorViewport = nullptr;
    QuadtreeViewer *quadtreeViewer = nullptr;
};
