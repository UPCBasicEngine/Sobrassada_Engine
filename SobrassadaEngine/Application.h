#pragma once

#include "Globals.h"
#include "Module.h"

#include <list>

class WindowModule;
class OpenGLModule;
class InputModule;
class ShaderModule;
class EditorUIModule;

// TMP: TEMPORAL JUST FOR HAVING A CAMERA TO RENDER
class CameraModule;
class DebugDrawModule;
class RenderTestModule;
class TextureModuleTest;

class EngineTimer;
class GameTimer;

class Application
{
  public:
    Application();
    ~Application();

    bool Init();
    update_status Update();
    bool ShutDown();

    WindowModule *GetWindowModule() { return windowModule; }
    OpenGLModule *GetOpenGLModule() { return openGLModule; }
    InputModule *GetInputModule() { return inputModule; }
    ShaderModule *GetShaderModule() { return shaderModule; }
    EditorUIModule *GetEditorUIModule() { return editorUIModule; };

    // TMP: TEMPORAL JUST FOR HAVING A CAMERA TO RENDER
    CameraModule *GetCameraModule() { return cameraModule; }
    DebugDrawModule *GetDebugDreawModule() { return debugDraw; }
    RenderTestModule *GetRenderTestModule() { return renderTest; }
    TextureModuleTest *GetTextureModuleTest() { return textureModuleTest; }

    GameTimer *GetGameTimer() { return gameTimer; }

  private:
    std::list<Module *> modules;

    WindowModule *windowModule           = nullptr;
    OpenGLModule *openGLModule           = nullptr;
    InputModule *inputModule             = nullptr;
    ShaderModule *shaderModule           = nullptr;
    EditorUIModule *editorUIModule       = nullptr;

    // TMP: TEMPORAL JUST FOR HAVING A CAMERA TO RENDER
    CameraModule *cameraModule           = nullptr;
    DebugDrawModule *debugDraw           = nullptr;
    RenderTestModule *renderTest         = nullptr;
    TextureModuleTest *textureModuleTest = nullptr;

    EngineTimer *engineTimer             = nullptr;
    GameTimer *gameTimer                 = nullptr;
};

extern Application *App;
