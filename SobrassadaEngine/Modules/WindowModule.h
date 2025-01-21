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
    SDL_DisplayMode &GetDesktopDisplayMode() const;
    float GetBrightness() const;

    void SetBrightness(const float brightness) const;
    void SetWidth(const unsigned int width);
    void SetHeight(const unsigned int height);
    bool SetFullscreen(bool fullscreen) const;
    void SetResizable(bool resizable) const;
    void SetBorderless(bool borderless) const;
    bool SetFullDesktop(bool fullDesktop) const;

  public:
    SDL_Window *window         = nullptr;
    SDL_Surface *screenSurface = nullptr;

  private:
    int windowWidth  = SCREEN_WIDTH;
    int windowHeight = SCREEN_HEIGHT;
};
