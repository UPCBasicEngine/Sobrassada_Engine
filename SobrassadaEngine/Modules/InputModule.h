#pragma once

#include "Module.h"

#include "Math/float2.h"
#include "SDL_scancode.h"
#include <SDL_gamecontroller.h>
#define MAX_CONTROLLERS 1
#include <functional>
#include <vector>

typedef unsigned __int8 Uint8;
#define NUM_MOUSE_BUTTONS 5

enum KeyState
{
    KEY_IDLE = 0,
    KEY_DOWN,
    KEY_REPEAT,
    KEY_UP
};

class InputModule : public Module
{
  public:
    InputModule();
    ~InputModule() override;

    bool Init() override;
    update_status PreUpdate(float deltaTime) override;
    bool ShutDown() override;

    const KeyState* GetKeyboard() const { return keyboard; }
    KeyState GetKey(int id) const { return keyboard[id]; } // avoid using this function

    const KeyState* GetMouseButtons() const { return mouseButtons; }
    KeyState GetMouseButtonDown(int id) const { return mouseButtons[id - 1]; }

    const float2& GetMouseMotion() const { return mouseMotion; };
    const float2& GetMousePosition() const { return mouse; };
    int GetMouseWheel() const { return mouseWheel; }

    float2 GetLeftStick() const { return controllerLeftStick; }
    float2 GetRightStick() const { return controllerRightStick; }              
    SDL_GameController* GetActiveController() const { return controllers[0]; } 

  private:
    KeyState* keyboard = NULL;
    KeyState mouseButtons[NUM_MOUSE_BUTTONS];
    float2 mouseMotion;
    float2 mouse;
    int mouseWheel = 0;

    SDL_GameController* controllers[MAX_CONTROLLERS] = {nullptr};
    float2 controllerLeftStick                       = float2(0, 0); 
    float2 controllerRightStick                      = float2(0, 0);
};
