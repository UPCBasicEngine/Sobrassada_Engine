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
    // GLOG("Init SDL input event system");

    bool returnStatus = true;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0)
    {
        GLOG("SDL_Init failed! SDL_Error: %s", SDL_GetError());
        returnStatus = false;
    }

    int numJoysticks = SDL_NumJoysticks();
    GLOG("Detected %d joystick(s)", numJoysticks);

    if (numJoysticks > 0 && SDL_IsGameController(0))
    {
        GLOG("Joystick 0 is a GameController");
        controllers[0] = SDL_GameControllerOpen(0);
        if (controllers[0] == nullptr)
        {
            GLOG("Could not open controller 0: %s", SDL_GetError());
            returnStatus = false;
        }
        else
        {
            GLOG("Controller 0 opened successfully!");
        }
    }
    else
    {
        GLOG("No valid GameController found at index 0");
    }

    return returnStatus;
}

update_status InputModule::PreUpdate(float deltaTime)
{
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    mouseMotion       = float2::zero;
    mouseWheel        = 0;

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (keys[i]) keyboard[i] = (keyboard[i] == KEY_IDLE) ? KEY_DOWN : KEY_REPEAT;
        else keyboard[i] = (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN) ? KEY_UP : KEY_IDLE;
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
                App->GetWindowModule()->WindowResized(sdlEvent.window.data1, sdlEvent.window.data2);
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
            break;
        }
    }

    if (controllers[0] != nullptr)
    {

        // Read analog stick axes
        Sint16 leftX           = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 leftY           = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_LEFTY);
        Sint16 rightX          = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_RIGHTX);
        Sint16 rightY          = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_RIGHTY);

        // Normalize stick values to [-1.0, 1.0], only if outside the deadzone
        controllerLeftStick.x  = fabs(leftX) > GAMEPAD_DEADZONE ? leftX / 32767.0f : 0.0f;
        controllerLeftStick.y  = fabs(leftY) > GAMEPAD_DEADZONE ? leftY / 32767.0f : 0.0f;
        controllerRightStick.x = fabs(rightX) > GAMEPAD_DEADZONE ? rightX / 32767.0f : 0.0f;
        controllerRightStick.y = fabs(rightY) > GAMEPAD_DEADZONE ? rightY / 32767.0f : 0.0f;

        // Log left stick movement if it’s significant
        if (fabs(controllerLeftStick.x) > 0.01f || fabs(controllerLeftStick.y) > 0.01f)
            GLOG("Left Stick: x=%.2f, y=%.2f", controllerLeftStick.x, controllerLeftStick.y);

        // Log right stick movement if it’s significant
        if (fabs(controllerRightStick.x) > 0.01f || fabs(controllerRightStick.y) > 0.01f)
            GLOG("Right Stick: x=%.2f, y=%.2f", controllerRightStick.x, controllerRightStick.y);

        // Read analog trigger values (L2 and R2)
        Sint16 triggerLeft  = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        Sint16 triggerRight = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

        // Normalize and log triggers if above noise threshold
        float normLT        = triggerLeft / 32767.0f;
        float normRT        = triggerRight / 32767.0f;

        if (normLT > 0.01f) GLOG("Left Trigger (LT): %.2f", normLT);
        if (normRT > 0.01f) GLOG("Right Trigger (RT): %.2f", normRT);

        // Log all buttons currently pressed
        for (int b = SDL_CONTROLLER_BUTTON_A; b < SDL_CONTROLLER_BUTTON_MAX; ++b)
        {
            if (SDL_GameControllerGetButton(controllers[0], (SDL_GameControllerButton)b))
            {
                const char* btnName = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)b);
                GLOG("Button pressed: %s", btnName ? btnName : "Unknown");
            }
        }
    }

    return UPDATE_CONTINUE;
}

bool InputModule::ShutDown()
{
    // GLOG("Quitting SDL input event subsystem");
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

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
