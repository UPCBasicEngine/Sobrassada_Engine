#pragma once

#include "imgui_internal.h"
#include "./Libs/ImGuizmo/ImGuizmo.h"
#include "Module.h"
#include "ResourceManagement/Resources/Resource.h"
#include "Transform.h"

#include <deque>
#include <string>
#include <unordered_map>

class EditorViewport;
class QuadtreeViewer;

class EditorUIModule : public Module
{
  public:
    EditorUIModule();
    ~EditorUIModule() override;

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    update_status Update(float deltaTime) override;
    update_status RenderEditor(float deltaTime) override;
    update_status PostUpdate(float deltaTime) override;
    bool ShutDown() override;

    bool RenderTransformWidget(Transform &localTransform, Transform &globalTransform, const Transform &parentTransform);
    bool RenderImGuizmo(Transform& gameObjectTransform);

    UID RenderResourceSelectDialog(const char *id, const std::unordered_map<std::string, UID> &availableResources);

  public:
    bool hierarchyMenu = true;
    bool inspectorMenu = true;

  private:
    void RenderBasicTransformModifiers(
        Transform &transform, bool &lockScaleAxis, bool &positionValueChanged, bool &rotationValueChanged,
        bool &scaleValueChanged
    );

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
    bool consoleMenu            = true;
    bool import             = false;
    bool load               = false;
    bool save               = false;
    bool loadScene          = false;
    bool editorSettingsMenu = false;
    bool quadtreeViewerViewport = false;
    bool closeApplication   = false;

    int maximumPlotData         = 50;
    std::deque<float> framerate;
    std::deque<float> frametime;

    int transformType = LOCAL;

    
    QuadtreeViewer *quadtreeViewer = nullptr;

    int width, height;

    std::string startPath;
    std::string libraryPath;
    
    ImGuizmo::OPERATION mCurrentGizmoOperation;
};