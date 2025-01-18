#pragma once

#include "Module.h"

#include "SDL.h"

class WindowModule : public Module
{
public:

	WindowModule();
	~WindowModule();

	bool Init() override;
	bool CleanUp() override;

	SDL_Window* window = NULL;
	SDL_Surface* screen_surface = NULL;
};

