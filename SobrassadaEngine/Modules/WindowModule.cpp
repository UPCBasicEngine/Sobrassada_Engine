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

		windowWidth = displayMode.w / 2;
		windowHeight = displayMode.h / 2;

		Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

		if (FULLSCREEN == true) flags |= SDL_WINDOW_FULLSCREEN;
		
		window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, flags);

		if (window == NULL)
		{
			GLOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			returnStatus = false;
		}
		else
		{
			//Get window surface
			screenSurface = SDL_GetWindowSurface(window);
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

void WindowModule::WindowResized(const unsigned int width, const unsigned int height)
{
	windowWidth = width;
	windowHeight = height;
}

SDL_DisplayMode& WindowModule::GetDesktopDisplayMode() const
{
	SDL_DisplayMode displayMode;
	SDL_GetDesktopDisplayMode(0, &displayMode);

	return displayMode;
}

float WindowModule::GetBrightness() const
{
	return SDL_GetWindowBrightness(window);
}

void WindowModule::SetBrightness(const float brightness) const
{
	SDL_SetWindowBrightness(window, brightness);
}

void WindowModule::SetWidth(const unsigned int width)
{
	windowWidth = width;
	SDL_SetWindowSize(window, windowWidth, windowHeight);
}

void WindowModule::SetHeight(const unsigned int height)
{
	windowHeight = height;
	SDL_SetWindowSize(window, windowWidth, windowHeight);
}

bool WindowModule::SetFullscreen(bool fullscreen) const
{
	return SDL_SetWindowFullscreen(window, fullscreen);
}

void WindowModule::SetResizable(bool resizable) const
{
	SDL_SetWindowResizable(window, resizable ? SDL_TRUE : SDL_FALSE);
}

void WindowModule::SetBorderless(bool borderless) const
{
	SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE);
}

bool WindowModule::SetFullDesktop(bool fullDesktop) const
{
	return SDL_SetWindowFullscreen(window, fullDesktop ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}
