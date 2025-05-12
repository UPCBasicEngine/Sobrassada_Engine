#pragma once

#include "Globals.h"
#include "Module.h"

#include <list>

class EngineConfig;
class WindowModule;
class OpenGLModule;
class ResourcesModule;
class InputModule;
class ShaderModule;
class LibraryModule;
class EditorUIModule;
class ProjectModule;
class SceneModule;
class CameraModule;
class DebugDrawModule;
class GameUIModule;
class ScriptModule;
class PhysicsModule;
class PathfinderModule;
class AudioModule;
class GameDebugUIModule;

class EngineTimer;
class GameTimer;

class SOBRASADA_API_ENGINE Application
{
  public:
    Application();
    ~Application();

    bool Init();
    update_status Update();
    bool ShutDown();

    WindowModule* GetWindowModule() { return windowModule; }
    OpenGLModule* GetOpenGLModule() { return openGLModule; }
    ResourcesModule* GetResourcesModule() { return resourcesModule; }

    InputModule* GetInputModule() { return inputModule; }
    ShaderModule* GetShaderModule() { return shaderModule; }
    LibraryModule* GetLibraryModule() { return libraryModule; }
    EditorUIModule* GetEditorUIModule() { return editorUIModule; };
    ProjectModule* GetProjectModule() { return projectModule; };
    SceneModule* GetSceneModule() { return sceneModule; }
    CameraModule* GetCameraModule() { return cameraModule; }
    DebugDrawModule* GetDebugDrawModule() { return debugDraw; }
    GameUIModule* GetGameUIModule() { return gameUIModule; }
    ScriptModule* GetScriptModule() { return scriptModule; }
    PhysicsModule* GetPhysicsModule() { return physicsModule; }
    PathfinderModule* GetPathfinderModule() { return pathModule; }
    AudioModule* GetAudioModule() { return audioModule; }
    GameDebugUIModule* GetGameDebugUIModule() { return gameDebugUI; }

    EngineTimer* GetEngineTimer() { return engineTimer; }
    GameTimer* GetGameTimer() { return gameTimer; }

    EngineConfig* GetEngineConfig() const { return engineConfig; }

  private:
    std::list<Module*> modules;

    WindowModule* windowModule       = nullptr;
    OpenGLModule* openGLModule       = nullptr;
    ResourcesModule* resourcesModule = nullptr;
    InputModule* inputModule         = nullptr;
    ShaderModule* shaderModule       = nullptr;
    LibraryModule* libraryModule     = nullptr;
    EditorUIModule* editorUIModule   = nullptr;
    ProjectModule* projectModule     = nullptr;
    SceneModule* sceneModule         = nullptr;
    CameraModule* cameraModule       = nullptr;
    DebugDrawModule* debugDraw       = nullptr;
    GameUIModule* gameUIModule       = nullptr;
    ScriptModule* scriptModule       = nullptr;
    PhysicsModule* physicsModule     = nullptr;
    PathfinderModule* pathModule     = nullptr;
    AudioModule* audioModule         = nullptr;
    GameDebugUIModule* gameDebugUI   = nullptr;

    EngineTimer* engineTimer         = nullptr;
    GameTimer* gameTimer             = nullptr;

    EngineConfig* engineConfig       = nullptr;
};

extern SOBRASADA_API_ENGINE Application* App;