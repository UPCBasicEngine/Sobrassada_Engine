#pragma once

#include "Module.h"

class OpenGLModule : public Module
{
public:

	OpenGLModule();
	~OpenGLModule();

	bool Init() override;
	update_status PreUpdate(float deltaTime) override;
	update_status Update(float deltaTime) override;
	update_status PostUpdate(float deltaTime) override;
	bool CleanUp() override;

	void WindowResized(unsigned width, unsigned height);

	void* GetContext() const { return context; }

private:
	void* context;

	float clearColorRed = DEFAULT_GL_CLEAR_COLOR_RED;
	float clearColorGreen = DEFAULT_GL_CLEAR_COLOR_GREEN;
	float clearColorBlue = DEFAULT_GL_CLEAR_COLOR_BLUE;
};

