#include "WindowModule.h"

WindowModule::WindowModule()
{
}

WindowModule::~WindowModule()
{
}

bool WindowModule::Init()
{
	GLOG("Init SDL window & surface");
	bool returnStatus = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		GLOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		returnStatus = false;
	}
	else
	{
		SDL_DisplayMode displayMode;
		SDL_GetDesktopDisplayMode(0, &displayMode);

		int width = displayMode.w / 2;
		int height = displayMode.h / 2;
		Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

		if (FULLSCREEN == true) flags |= SDL_WINDOW_FULLSCREEN;
		
		window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);

		if (window == NULL)
		{
			GLOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			returnStatus = false;
		}
		else
		{
			//Get window surface
			screen_surface = SDL_GetWindowSurface(window);
		}
	}

	return returnStatus;
}

bool WindowModule::ShutDown()
{
	GLOG("Destroying SDL window and quitting all SDL systems");

	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	SDL_Quit();
	return true;
}
