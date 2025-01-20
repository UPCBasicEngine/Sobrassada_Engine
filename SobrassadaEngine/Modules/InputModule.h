#pragma once

#include "Module.h"
 
#include "SDL_scancode.h"
#include "Math/float2.h"
#include <list>
#include <unordered_map>
#include <functional>

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
	~InputModule();

	bool Init() override;
	update_status PreUpdate(float deltaTime) override;
	bool ShutDown();

	void SubscribeToEvent(int keyEvent, std::function<void(void)>& functionCallback);

	KeyState GetKey(int id) const
	{
		return keyboard[id];
	}

	KeyState GetMouseButtonDown(int id) const
	{
		return mouseButtons[id - 1];
	}

	const float2& GetMouseMotion() const { return mouseMotion; };
	const float2& GetMousePosition() const { return mouse; };
	int GetMouseWheel() const { return mouseWheel; }

private:

	KeyState* keyboard = NULL;
	KeyState mouseButtons[NUM_MOUSE_BUTTONS];
	float2 mouseMotion;
	float2 mouse;
	int mouseWheel = 0;

	std::unordered_map<int, std::list<std::function<void(void)>>> subscribedCallbacks;
};

