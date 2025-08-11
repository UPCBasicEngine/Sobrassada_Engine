#include "WindowModule.h"

#include "Application.h"
#include "GameUIModule.h"
#include "OpenGLModule.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "Framebuffer.h"
#include "ProjectModule.h"

WindowModule::WindowModule()
{
}

WindowModule::~WindowModule()
{
}

bool WindowModule::Init()
{
    //GLOG("Init SDL window & surface");
    bool returnStatus = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        GLOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
        returnStatus = false;
    }
    else
    {
        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(0, &displayMode);

        windowWidth  = displayMode.w / 2;
        windowHeight = displayMode.h / 2;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;

        if (FULLSCREEN == true) flags |= SDL_WINDOW_FULLSCREEN;
        else if (FULL_DESKTOP == true) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        else if (BORDERLESS == true) flags |= SDL_WINDOW_BORDERLESS;

        if (RESIZABLE == true) flags |= SDL_WINDOW_RESIZABLE;

        window = SDL_CreateWindow(
            (App->GetProjectModule()->GetLoadedProjectName() + " - " + TITLE).c_str(), SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, flags
        );

        if (window == NULL)
        {
            GLOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            returnStatus = false;
        }
        else
        {
            screenSurface = SDL_GetWindowSurface(window);
            vsync         = VSYNC;
        }
    }

    return returnStatus;
}

bool WindowModule::ShutDown()
{
    //GLOG("Destroying SDL window and quitting all SDL systems");

    if (window != NULL)
    {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return true;
}

void WindowModule::WindowResized(const unsigned int width, const unsigned int height)
{
    windowWidth  = width;
    windowHeight = height;
    App->GetOpenGLModule()->GetGBuffer()->Resize(width, height);
    App->GetOpenGLModule()->GetSsao()->Resize(width, height);
    App->GetOpenGLModule()->GetFramebuffer()->Resize(width, height);

    App->GetGameUIModule()->OnWindowResize(width, height);
}

SDL_DisplayMode& WindowModule::GetDesktopDisplayMode()
{
    SDL_GetDesktopDisplayMode(0, &displayMode);
    return displayMode;
}

void WindowModule::SetWidth(const unsigned int width)
{
    windowWidth = width;
    SDL_SetWindowSize(window, windowWidth, windowHeight);
}

void WindowModule::SetHeight(const unsigned int height)
{
    windowHeight = height;
    SDL_SetWindowSize(window, windowWidth, windowHeight);
}

void WindowModule::UpdateProjectNameInWindowTitle(const std::string& newProjectName) const
{
    SDL_SetWindowTitle(window, (newProjectName + " - " + TITLE).c_str());
}
