#pragma once

#include "Module.h"
#include "Globals.h"

#include<list>

class WindowModule;
class OpenGLModule;
class InputModule;
class ShaderModule;
class EditorUIModule;

class Application
{
public:
	Application();
	~Application();

	bool Init();
	update_status Update(float deltaTime);
	bool ShutDown();

	WindowModule* GetWindowModule() { return windowModule; }
	OpenGLModule* GetOpenGLModule() { return openGLModule; }
	InputModule* GetInputModule() { return inputModule; }
	ShaderModule* GetShaderModule() { return shaderModule; }
	EditorUIModule* GetEditorUIModule() { return editorUIModule; };

private:
	std::list<Module*> modules;
	
	WindowModule* windowModule = nullptr;
	OpenGLModule* openGLModule = nullptr;
	InputModule* inputModule = nullptr;
	ShaderModule* shaderModule = nullptr;
	EditorUIModule* editorUIModule = nullptr;

	uint32_t previousElapsedTime = 0;
};

extern Application* App;
