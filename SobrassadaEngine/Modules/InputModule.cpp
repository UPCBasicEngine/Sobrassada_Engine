#include "InputModule.h"

#include "Application.h"
#include "FileSystem.h"
#include "GameTimer.h"
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
    memset(controllerButtons, KEY_IDLE, sizeof(KeyState) * SDL_CONTROLLER_BUTTON_MAX);
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

    return returnStatus;
}

update_status InputModule::PreUpdate(float deltaTime)
{
    bool paused = App->GetGameTimer()->IsPaused();
    if (wasPausedLastFrame && !paused)
    {
        skipNextInputFrame = true;
        ClearTransientStates();
        SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);
    }
    wasPausedLastFrame = paused;

    /*** KEYBOARD AND MOUSE ***/
    const Uint8* keys  = SDL_GetKeyboardState(NULL);
    mouseMotion        = float2::zero;
    mouseWheel         = 0;

    if (skipNextInputFrame)
    {
        // Prevent a new KEY_DOWN: convert held keys to REPEAT, others to IDLE
        for (int i = 0; i < MAX_KEYS; ++i)
            keyboard[i] = keys[i] ? KEY_REPEAT : KEY_IDLE;

        // Clear mouse buttons
        for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
            mouseButtons[i] = KEY_IDLE;

        // Convert held controller buttons to REPEAT
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
            controllerButtons[i] = SDL_GameControllerGetButton(controllers[0], static_cast<SDL_GameControllerButton>(i))
                                     ? KEY_REPEAT
                                     : KEY_IDLE;

        skipNextInputFrame = false;
        return UPDATE_CONTINUE;
    }

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (keys[i])
        {
            keyboard[i]     = (keyboard[i] == KEY_IDLE) ? KEY_DOWN : KEY_REPEAT;
            isUsingKeyboard = true;
        }
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
            isUsingKeyboard                          = true;
            mouseButtons[sdlEvent.button.button - 1] = KEY_DOWN;
            break;
        case SDL_MOUSEBUTTONUP:
            isUsingKeyboard                          = true;
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
        case SDL_CONTROLLERDEVICEADDED:
            OnControllerConnected();
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            OnControllerDisconnected();
            break;
        }
    }

    if (controllers[0] == nullptr) return UPDATE_CONTINUE;

    /*** CONTROLLER ***/
    const Sint16 leftX  = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_LEFTX);
    const Sint16 leftY  = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_LEFTY);
    const Sint16 rightX = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_RIGHTX);
    const Sint16 rightY = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_RIGHTY);

    // Normalize stick values to [-1.0, 1.0], only if outside the deadzone
    if (fabs(leftX) + fabs(leftY) > GAMEPAD_DEADZONE)
    {
        controllerLeftStick.x = leftX / 32767.0f;
        controllerLeftStick.y = leftY / 32767.0f;
        isUsingKeyboard       = false;
        // GLOG("Left Stick: x=%.2f, y=%.2f", controllerLeftStick.x, controllerLeftStick.y);
    }
    else
    {
        controllerLeftStick.x = 0.0f;
        controllerLeftStick.y = 0.0f;
    }

    if (fabs(rightX) + fabs(rightY) > GAMEPAD_DEADZONE)
    {
        controllerRightStick.x = rightX / 32767.0f;
        controllerRightStick.y = rightY / 32767.0f;
        isUsingKeyboard        = false;
        // GLOG("Right Stick: x=%.2f, y=%.2f", controllerRightStick.x, controllerRightStick.y);
    }
    else
    {
        controllerRightStick.x = 0.0f;
        controllerRightStick.y = 0.0f;
    }

    // Read analog trigger values (L2 and R2)
    const Sint16 triggerLeft  = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    const Sint16 triggerRight = SDL_GameControllerGetAxis(controllers[0], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

    // Normalize and log triggers if above noise threshold
    const float normLT        = triggerLeft / 32767.0f;
    const float normRT        = triggerRight / 32767.0f;

    if (normLT > 0.01f)
    {
        isUsingKeyboard    = false;
        leftTrigger.first  = (leftTrigger.first == KEY_IDLE) ? KEY_DOWN : KEY_REPEAT;
        leftTrigger.second = normLT;
        // GLOG("Left Trigger (LT): %.2f", normLT);
    }
    else
    {
        leftTrigger.first = (leftTrigger.first == KEY_REPEAT || leftTrigger.first == KEY_DOWN) ? KEY_UP : KEY_IDLE;
    }

    if (normRT > 0.01f)
    {
        isUsingKeyboard     = false;
        rightTrigger.first  = (rightTrigger.first == KEY_IDLE) ? KEY_DOWN : KEY_REPEAT;
        rightTrigger.second = normRT;
        // GLOG("Right Trigger (RT): %.2f", normRT);
    }
    else
    {
        rightTrigger.first = (rightTrigger.first == KEY_REPEAT || rightTrigger.first == KEY_DOWN) ? KEY_UP : KEY_IDLE;
    }

    // Log all buttons currently pressed
    for (int i = SDL_CONTROLLER_BUTTON_A; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
    {
        if (SDL_GameControllerGetButton(controllers[0], (SDL_GameControllerButton)i))
        {
            controllerButtons[i] = (controllerButtons[i] == KEY_IDLE) ? KEY_DOWN : KEY_REPEAT;
            isUsingKeyboard      = false;
        }
        else
        {
            controllerButtons[i] =
                (controllerButtons[i] == KEY_REPEAT || controllerButtons[i] == KEY_DOWN) ? KEY_UP : KEY_IDLE;
        }

        // if (SDL_GameControllerGetButton(controllers[0], (SDL_GameControllerButton)i))
        //{
        //     const char* btnName = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)i);
        //     GLOG("Button pressed: %s", btnName ? btnName : "Unknown");
        // }
    }

    return UPDATE_CONTINUE;
}

bool InputModule::ShutDown()
{
    for (int i = 0; i < MAX_CONTROLLERS; ++i)
    {
        if (controllers[i])
        {
            SDL_GameControllerClose(controllers[i]);
            controllers[i] = nullptr;
        }
    }

    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

    return true;
}

void InputModule::OnControllerConnected()
{
    int numJoysticks = SDL_NumJoysticks();
    GLOG("Detected %d joystick(s)", numJoysticks);

    if (numJoysticks > 0 && SDL_IsGameController(0))
    {
        GLOG("Joystick 0 is a GameController");
        // Override always the controller at 0, we don't have coop
        controllers[0] = SDL_GameControllerOpen(0);
        if (controllers[0] == nullptr)
        {
            GLOG("Could not open controller 0: %s", SDL_GetError());
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
}

void InputModule::OnControllerDisconnected()
{
    if (controllers[0])
    {
        GLOG("Disconnect controller")
        SDL_GameControllerClose(controllers[0]);
        controllers[0] = nullptr;
    }
}

void InputModule::ClearTransientStates()
{
    // KEY_DOWN and KEY_UP to KEY_IDLE
    for (int i = 0; i < MAX_KEYS; ++i)
        if (keyboard[i] == KEY_DOWN || keyboard[i] == KEY_UP) keyboard[i] = KEY_IDLE;

    for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
        if (mouseButtons[i] == KEY_DOWN || mouseButtons[i] == KEY_UP) mouseButtons[i] = KEY_IDLE;

    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
        if (controllerButtons[i] == KEY_DOWN || controllerButtons[i] == KEY_UP) controllerButtons[i] = KEY_IDLE;

    if (leftTrigger.first == KEY_DOWN || leftTrigger.first == KEY_UP) leftTrigger.first = KEY_IDLE;
    if (rightTrigger.first == KEY_DOWN || rightTrigger.first == KEY_UP) rightTrigger.first = KEY_IDLE;
}
