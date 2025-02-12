#include "GameTimer.h"
#include "Application.h"
#include "Globals.h"
#include "SDL.h"

GameTimer::GameTimer()
    : EngineTimer(), isPaused(false), frameCount(0), unscaledTime(0), unscaledDeltaTime(0), timeScale(1)
{
}

GameTimer::~GameTimer() {}

float GameTimer::Tick()
{
    if (!isPaused && isEnabled) return UpdateTimes();
    else return 0;
}

void GameTimer::Pause() { isPaused = true; }

void GameTimer::Reset()
{
    time              = 0;
    unscaledTime      = 0;
    startTime         = 0;
    deltaTime         = 0;
    unscaledDeltaTime = 0;
    frameCount        = 0;
    isPaused          = false;
}

float GameTimer::Step()
{
    if (isEnabled && isPaused) return UpdateTimes();
    else return 0;
}

float GameTimer::TicksSinceStartup() const { return SDL_GetTicks() - startTime; }

float GameTimer::UpdateTimes()
{
    ++frameCount;
    deltaTime          = (TicksSinceStartup() - unscaledTime) * timeScale;
    unscaledDeltaTime  = TicksSinceStartup() - unscaledTime;
    time              += deltaTime;
    unscaledTime       = TicksSinceStartup();
    return deltaTime;
}