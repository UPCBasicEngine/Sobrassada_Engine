#include "Application.h"

#include "AudioModule.h"
#include "CameraModule.h"
#include "ComponentUtils.h"
#include "Config/EngineConfig.h"
#include "DebugDrawModule.h"
#include "EditorUIModule.h"
#include "EngineTimer.h"
#include "Framebuffer.h"
#include "GameDebugUIModule.h"
#include "GameTimer.h"
#include "GameUIModule.h"
#include "InputModule.h"
#include "LibraryModule.h"
#include "OpenGLModule.h"
#include "PathfinderModule.h"
#include "PhysicsModule.h"
#include "ProjectModule.h"
#include "ResourcesModule.h"
#include "SceneModule.h"
#include "ScriptModule.h"
#include "ShaderModule.h"
#include "WindowModule.h"

#ifdef OPTICK
#include "optick.h"
#endif

Application::Application()
{
    engineConfig = new EngineConfig();

    modules.push_back(scriptModule = new ScriptModule());
    modules.push_back(projectModule = new ProjectModule());
    modules.push_back(windowModule = new WindowModule());
    modules.push_back(openGLModule = new OpenGLModule());
    modules.push_back(libraryModule = new LibraryModule());
    modules.push_back(resourcesModule = new ResourcesModule());
    modules.push_back(inputModule = new InputModule());
    modules.push_back(shaderModule = new ShaderModule());
    modules.push_back(physicsModule = new PhysicsModule());
    modules.push_back(audioModule = new AudioModule());
    modules.push_back(sceneModule = new SceneModule());
    modules.push_back(pathModule = new PathfinderModule());
    modules.push_back(gameUIModule = new GameUIModule());
    modules.push_back(cameraModule = new CameraModule());
    modules.push_back(debugDraw = new DebugDrawModule());
    modules.push_back(editorUIModule = new EditorUIModule());
    modules.push_back(gameDebugUI = new GameDebugUIModule());

    engineTimer = new EngineTimer();
    engineTimer->Start();
    gameTimer = new GameTimer();
}

Application::~Application()
{
    for (std::list<Module*>::iterator it = modules.begin(); it != modules.end(); ++it)
    {
        delete *it;
    }
    delete engineConfig;
    engineConfig = nullptr;
}

bool Application::Init()
{
    bool returnStatus = true;

    for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && returnStatus; ++it)
        returnStatus = (*it)->Init();

    return returnStatus;
}

update_status Application::Update()
{
    const float deltaTime = engineTimer->Tick() / 1000.0f;
    gameTimer->Tick();

    update_status returnStatus = UPDATE_CONTINUE;
    {
#ifdef OPTICK
        OPTICK_CATEGORY("Application::PreUpdate", Optick::Category::GameLogic)
#endif
        for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
             ++it)
            returnStatus = (*it)->PreUpdate(deltaTime);
    }
    {
#ifdef OPTICK
        OPTICK_CATEGORY("Application::Update", Optick::Category::GameLogic)
#endif
        for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
             ++it)
            returnStatus = (*it)->Update(deltaTime);
    }
    {
#ifdef OPTICK
        OPTICK_CATEGORY("Application::Render", Optick::Category::Rendering)
#endif
        for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
             ++it)
            returnStatus = (*it)->Render(deltaTime);
    }

    {
#ifdef OPTICK
        OPTICK_CATEGORY("Application::RenderEditor", Optick::Category::Rendering)
#endif
#ifndef GAME
        // Unbinding frame buffer so ui gets rendered
        App->GetOpenGLModule()->GetFramebuffer()->Unbind();
        for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
             ++it)
            returnStatus = (*it)->RenderEditor(deltaTime);
#endif
#ifdef GAME
        App->GetOpenGLModule()->GetFramebuffer()->Unbind();
        GetGameDebugUIModule()->RenderEditor(deltaTime);
#endif
    }

    {
#ifdef OPTICK
        OPTICK_CATEGORY("Application::PostUpdate", Optick::Category::GameLogic)
#endif
        for (std::list<Module*>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
             ++it)
            returnStatus = (*it)->PostUpdate(deltaTime);
    }
    return returnStatus;
}

bool Application::ShutDown()
{
    bool returnStatus = true;

    for (std::list<Module*>::reverse_iterator it = modules.rbegin(); it != modules.rend() && returnStatus; ++it)
        returnStatus = (*it)->ShutDown();

    delete engineTimer;
    delete gameTimer;

    return returnStatus;
}
