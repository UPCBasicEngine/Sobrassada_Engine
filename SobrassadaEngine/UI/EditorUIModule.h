#pragma once

#include "ImGuizmo.h"
#include "Module.h"

#include "ResourceManagement/Resources/Resource.h"
#include "Scene/AABBUpdatable.h"
#include "Transform.h"

#include <deque>
#include <map>
#include <string>
#include <unordered_map>

//namespace ImGuizmo {
//struct matrix_t;}

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

    bool RenderTransformWidget(Transform &localTransform, Transform &globalTransform, const Transform &parentTransform);

    UID RenderResourceSelectDialog(const char *id, const std::unordered_map<std::string, UID> &availableResources);

    void DrawGizmos(const float4x4 &view, const float4x4 &proj, const float4x4 &model);

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

    EditorViewport *editorViewport = nullptr;
    QuadtreeViewer *quadtreeViewer = nullptr;

    int width, height;

    std::string startPath;
    std::string libraryPath;

    //static ImGuizmo::OPERATION mCurrentGizmoOperation;
    //ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::LOCAL;
};