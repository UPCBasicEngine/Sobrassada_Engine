#include "Application.h"

#include "EditorUIModule.h"
#include "InputModule.h"
#include "OpenGLModule.h"
#include "ShaderModule.h"
#include "WindowModule.h"

// TMP: TEMPORAL JUST FOR HAVING A CAMERA TO RENDER
#include "CameraModule.h"
#include "DebugDrawModule.h"
#include "RenderTestModule.h"
#include "TextureModuleTest.h"

Application::Application()
{
    modules.push_back(windowModule = new WindowModule());

    // TMP: TEMPORAL JUST FOR HAVING A CAMERA TO RENDER
    modules.push_back(textureModuleTest = new TextureModuleTest());

    modules.push_back(openGLModule = new OpenGLModule());
    modules.push_back(inputModule = new InputModule());
    modules.push_back(shaderModule = new ShaderModule());

    // TMP: TEMPORAL JUST FOR HAVING A CAMERA TO RENDER
    modules.push_back(cameraModule = new CameraModule());
    modules.push_back(debugDraw = new DebugDrawModule());
    modules.push_back(renderTest = new RenderTestModule());

    modules.push_back(editorUIModule = new EditorUIModule());
}

Application::~Application()
{
    for (std::list<Module *>::iterator it = modules.begin(); it != modules.end(); ++it)
    {
        delete *it;
    }
}

bool Application::Init()
{
    bool returnStatus = true;

    for (std::list<Module *>::iterator it = modules.begin(); it != modules.end() && returnStatus; ++it)
        returnStatus = (*it)->Init();

    return returnStatus;
}

update_status Application::Update(float deltaTime)
{
    update_status returnStatus = UPDATE_CONTINUE;

    for (std::list<Module *>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
         ++it)
        returnStatus = (*it)->PreUpdate(deltaTime);

    for (std::list<Module *>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
         ++it)
        returnStatus = (*it)->Update(deltaTime);

    for (std::list<Module *>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
         ++it)
        returnStatus = (*it)->Render(deltaTime);

    for (std::list<Module *>::iterator it = modules.begin(); it != modules.end() && returnStatus == UPDATE_CONTINUE;
         ++it)
        returnStatus = (*it)->PostUpdate(deltaTime);

    return returnStatus;
}

bool Application::ShutDown()
{
    bool returnStatus = true;

    for (std::list<Module *>::reverse_iterator it = modules.rbegin(); it != modules.rend() && returnStatus; ++it)
        returnStatus = (*it)->ShutDown();

    return returnStatus;
}
