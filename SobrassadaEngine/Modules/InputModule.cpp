#include "InputModule.h"
#include "Application.h"

#include "SDL.h"
#include "imgui_impl_sdl2.h"

#define MAX_KEYS 300

InputModule::InputModule()
{
    keyboard = new KeyState[MAX_KEYS];
    memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
    memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

InputModule::~InputModule() { RELEASE_ARRAY(keyboard); }

bool InputModule::Init()
{
    GLOG("Init SDL input event system");
    bool returnStatus = true;
    SDL_Init(0);

    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
    {
        GLOG("SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
        returnStatus = false;
    }

    return returnStatus;
}

update_status InputModule::PreUpdate(float deltaTime)
{
    // Checking and updating keyboard key states
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
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

            mouse.x       = sdlEvent.motion.x / 2.f;
            mouse.y       = sdlEvent.motion.y / 2.f;
            break;
        case SDL_MOUSEWHEEL:
            mouseWheel = sdlEvent.wheel.y;
            break;
            // case SDL_DROPFILE:
            //	std::string filePath = std::string(sdlEvent.drop.file);
            //	int fileExtensionPosition = (int)filePath.find_last_of('.');

            //	std::string fileExtension = filePath.substr(fileExtensionPosition, filePath.length());
            //	if (fileExtension == ".ppm" || fileExtension == ".png" || fileExtension == ".jpg")
            //App->GetModelViewerModule()->LoadTexture(filePath.c_str()); 	else if (fileExtension == ".gltf")
            //App->GetModelViewerModule()->LoadModel(filePath.c_str());
        }
    }

    // TESTING EVENT DISPATCHER
    if (keyboard[SDL_SCANCODE_F] == KEY_DOWN)
    {
        for (const auto it : subscribedCallbacks[SDL_SCANCODE_F])
        {
            it();
        }
    }

    return UPDATE_CONTINUE;
}

bool InputModule::ShutDown()
{
    GLOG("Quitting SDL input event subsystem");
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    return true;
}

void InputModule::SubscribeToEvent(int keyEvent, std::function<void(void)> &functionCallback)
{
    auto eventIterator = subscribedCallbacks.find(keyEvent);
    if (eventIterator != subscribedCallbacks.end())
    {
        eventIterator->second.push_back(functionCallback);
    }
    else
    {
        auto newList = std::list<std::function<void(void)>>();
        newList.push_back(functionCallback);
        subscribedCallbacks.insert({keyEvent, newList});
    }
}
