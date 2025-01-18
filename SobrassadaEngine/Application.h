#pragma once

#include "Module.h"
#include "Globals.h"

#include<list>

class WindowModule;
class OpenGLModule;
class InputModule;

class Application
{
public:
	Application();
	~Application();

	bool Init();
	update_status Update(float deltaTime);
	bool CleanUp();

	WindowModule* GetWindowModule() { return windowModule; }
	OpenGLModule* GetOpenGLModule() { return openGLModule; }
	InputModule* GetInputModule() { return inputModule; }

private:
	std::list<Module*> modules;
	
	WindowModule* windowModule = nullptr;
	OpenGLModule* openGLModule = nullptr;
	InputModule* inputModule = nullptr;

	uint32_t previousElapsedTime = 0;
};

extern Application* App;
