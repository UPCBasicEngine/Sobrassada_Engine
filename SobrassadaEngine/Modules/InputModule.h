#pragma once

#include "Module.h"

#include "Math/float2.h"
#include "SDL_scancode.h"
#include <SDL_gamecontroller.h>
#define MAX_CONTROLLERS  4       // Controllers connected
#define GAMEPAD_DEADZONE 8000.0f // Deadzone threshold to avoid detecting unintentional stick movement
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
    const KeyState* GetControllerButtons() const { return controllerButtons; }
    const std::pair<KeyState, float>& GetLeftTrigger() const { return leftTrigger; }
    const std::pair<KeyState, float>& GetRightTrigger() const { return rightTrigger; }

    bool IsUsingKeyboard() const { return isUsingKeyboard; }

  private:
    KeyState* keyboard = NULL;
    KeyState mouseButtons[NUM_MOUSE_BUTTONS];
    float2 mouseMotion                               = float2::zero;
    float2 mouse                                     = float2::zero;
    int mouseWheel                                   = 0;

    SDL_GameController* controllers[MAX_CONTROLLERS] = {nullptr};
    float2 controllerLeftStick                       = float2::zero;
    float2 controllerRightStick                      = float2::zero;
    KeyState controllerButtons[SDL_CONTROLLER_BUTTON_MAX];
    std::pair<KeyState, float> leftTrigger;
    std::pair<KeyState, float> rightTrigger;

    bool isUsingKeyboard = true;
};
