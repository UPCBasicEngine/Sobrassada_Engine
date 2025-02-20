#pragma once

#include "imgui_internal.h"
#include "./Libs/ImGuizmo/ImGuizmo.h"
#include "Module.h"
#include "ResourceManagement/Resources/Resource.h"
#include "Transform.h"

#include "SDL.h"
#include <deque>
#include <string>
#include <unordered_map>

struct CPUFeature
{
    SDL_bool (*check)();
    const char *name;
};

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

    void ImportDialog(bool &import);
    void GetFilesSorted(const std::string &currentPath, std::vector<std::string> &files);
    void LoadDialog(bool &load);
    void SaveDialog(bool &save);
    void Console(bool &consoleMenu) const;
    void About(bool &aboutMenu) const;

  private:
    bool consoleMenu            = true;
    bool import             = false;
    bool load               = false;
    bool save               = false;
    bool aboutMenu          = false;
    bool editorSettingsMenu = false;
    bool closeApplication   = false;

    int maxFPS              = 60;
    int maximumPlotData     = 50;
    std::deque<float> framerate;
    std::deque<float> frametime;

    int transformType = LOCAL;

    int width, height;

    std::string startPath;
    std::string libraryPath;
    
    ImGuizmo::OPERATION mCurrentGizmoOperation;
};