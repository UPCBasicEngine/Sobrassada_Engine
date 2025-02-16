#pragma once

#include "Module.h"

#include "SDL.h"

class WindowModule : public Module
{
  public:
    WindowModule();
    ~WindowModule();

    bool Init() override;
    bool ShutDown() override;

    void WindowResized(const unsigned int width, const unsigned int height);

    int GetWidth() const { return windowWidth; }
    int GetHeight() const { return windowHeight; }
    SDL_DisplayMode &GetDesktopDisplayMode();
    float GetBrightness() const { return SDL_GetWindowBrightness(window); }

    void SetWidth(const unsigned int width);
    void SetHeight(const unsigned int height);

    void SetBrightness(const float brightness) const { SDL_SetWindowBrightness(window, brightness); }
    bool SetFullscreen(bool fullscreen) const { return SDL_SetWindowFullscreen(window, fullscreen); }
    void SetResizable(bool resizable) const { SDL_SetWindowResizable(window, resizable ? SDL_TRUE : SDL_FALSE); }
    void SetBorderless(bool borderless) const { SDL_SetWindowBordered(window, borderless ? SDL_FALSE : SDL_TRUE); }
    bool SetFullDesktop(bool fullDesktop) const
    {
        return SDL_SetWindowFullscreen(window, fullDesktop ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
    void SetVsync(bool vsync) const { SDL_GL_SetSwapInterval(vsync ? 1 : 0); }

  public:
    SDL_Window* window         = nullptr;
    SDL_Surface* screenSurface = nullptr;

  private:
    SDL_DisplayMode displayMode;
    int windowWidth  = SCREEN_WIDTH;
    int windowHeight = SCREEN_HEIGHT;
};
