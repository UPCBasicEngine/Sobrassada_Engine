#include "InputModule.h"

#include "Application.h"
#include "FileSystem.h"
#include "SceneImporter.h"
#include "SceneModule.h"
#include "WindowModule.h"

#include "SDL.h"
#include "imgui_impl_sdl2.h"

#define MAX_KEYS 300

InputModule::InputModule()
{
    keyboard = new KeyState[MAX_KEYS];
    memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
    memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

InputModule::~InputModule()
{
    RELEASE_ARRAY(keyboard);
}

bool InputModule::Init()
{
    //GLOG("Init SDL input event system");
    bool returnStatus = true;
    SDL_Init(0);

    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
    {
        GLOG("SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
        returnStatus = false;
    }

    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks && i < MAX_CONTROLLERS; ++i)
    {
        if (SDL_IsGameController(i))
        {
            controllers[i] = SDL_GameControllerOpen(i);
            if (controllers[i] == nullptr)
            {
                GLOG("Could not open gamecontroller %d: %s", i, SDL_GetError());
            }
        }
    }

    return returnStatus;
}

update_status InputModule::PreUpdate(float deltaTime)
{
    // Checking and updating keyboard key states
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    mouseMotion.x     = 0;
    mouseMotion.y     = 0;
    mouseWheel        = 0;

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (keys[i] == 1)
        {
            if (keyboard[i] == KEY_IDLE) keyboard[i] = KEY_DOWN;
            else keyboard[i] = KEY_REPEAT;
        }
        else
        {
            if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN) keyboard[i] = KEY_UP;
            else keyboard[i] = KEY_IDLE;
        }
    }

    for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
    {
        if (mouseButtons[i] == KEY_DOWN) mouseButtons[i] = KEY_REPEAT;

        if (mouseButtons[i] == KEY_UP) mouseButtons[i] = KEY_IDLE;
    }

    SDL_Event sdlEvent;

    while (SDL_PollEvent(&sdlEvent) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
        switch (sdlEvent.type)
        {
        case SDL_QUIT:
            return UPDATE_STOP;
        case SDL_WINDOWEVENT:
            if (sdlEvent.window.event == SDL_WINDOWEVENT_RESIZED ||
                sdlEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                App->GetWindowModule()->WindowResized(sdlEvent.window.data1, sdlEvent.window.data2);
                // App->GetOpenGLModule()->WindowResized(sdlEvent.window.data1, sdlEvent.window.data2);
                // App->GetEditorModule()->SetNewScreenSize(sdlEvent.window.data1, sdlEvent.window.data2);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouseButtons[sdlEvent.button.button - 1] = KEY_DOWN;

            break;

        case SDL_MOUSEBUTTONUP:
            mouseButtons[sdlEvent.button.button - 1] = KEY_UP;
            break;
        case SDL_MOUSEMOTION:
            mouseMotion.x = sdlEvent.motion.xrel / 2.f;
            mouseMotion.y = sdlEvent.motion.yrel / 2.f;

            mouse.x       = static_cast<float>(sdlEvent.motion.x);
            mouse.y       = static_cast<float>(sdlEvent.motion.y);
            break;
        case SDL_MOUSEWHEEL:
            mouseWheel = sdlEvent.wheel.y;
            break;
        case SDL_DROPFILE:
            SceneImporter::Import(sdlEvent.drop.file);
        }
    }

    for (int i = 0; i < MAX_CONTROLLERS; ++i)
    {
        if (controllers[i])
        {
            float deadzone         = 8000.0f;

            Sint16 leftX           = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 leftY           = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_LEFTY);
            Sint16 rightX          = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_RIGHTX);
            Sint16 rightY          = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_RIGHTY);

            controllerLeftStick.x  = fabs(leftX) > deadzone ? leftX / 32767.0f : 0.0f;
            controllerLeftStick.y  = fabs(leftY) > deadzone ? leftY / 32767.0f : 0.0f;

            controllerRightStick.x = fabs(rightX) > deadzone ? rightX / 32767.0f : 0.0f;
            controllerRightStick.y = fabs(rightY) > deadzone ? rightY / 32767.0f : 0.0f;

            if (fabs(controllerLeftStick.x) > 0.01f || fabs(controllerLeftStick.y) > 0.01f)
            {
                GLOG("Left Stick: x=%.2f, y=%.2f", controllerLeftStick.x, controllerLeftStick.y);
            }

            break;
        }
    }


    return UPDATE_CONTINUE;
}

bool InputModule::ShutDown()
{
    //GLOG("Quitting SDL input event subsystem");
    SDL_QuitSubSystem(SDL_INIT_EVENTS);

    for (int i = 0; i < MAX_CONTROLLERS; ++i)
    {
        if (controllers[i])
        {
            SDL_GameControllerClose(controllers[i]);
            controllers[i] = nullptr;
        }
    }

    return true;
}
