#include "GameTimer.h"
#include "Application.h"
#include "Globals.h"
#include "SDL.h"

GameTimer::GameTimer()
    : EngineTimer(), isPaused(false), frameCount(0), unscaledTime(0), unscaledDeltaTime(0), timeScale(1),
      referenceTime(0), pausedTime(0)
{
}

GameTimer::~GameTimer() {}

void GameTimer::Start()
{
    EngineTimer::Start();
    isPaused      = false;
    referenceTime = startTime;
    unstoppableDeltaTime = startTime;
}

float GameTimer::Tick()
{
    float delta = 0;

    if (isEnabled)
    {
        unstoppableDeltaTime = TicksSinceStartup() - unstoppableTime;
        unstoppableTime      = TicksSinceStartup();

        if (!isPaused) delta = UpdateTimes();
        else
        {
            referenceTime += unstoppableDeltaTime;
            pausedTime     = TicksSinceStartup();
        }
    }

    return delta;
}

void GameTimer::TogglePause()
{
    if (isPaused)
    {
        isPaused = false;
    }
    else
    {
        isPaused   = true;
        pausedTime = TicksSinceStartup();
    }
}

void GameTimer::Reset()
{
    time                 = 0;
    unscaledTime         = 0;
    startTime            = 0;
    deltaTime            = 0;
    unscaledDeltaTime    = 0;
    frameCount           = 0;
    referenceTime        = 0;
    pausedTime           = 0;
    isPaused             = false;
    isEnabled            = false;

    unstoppableTime      = 0;
    unstoppableDeltaTime = 0;
}

float GameTimer::Step()
{
    if (isEnabled && isPaused) return UpdateTimes();
    else return 0;
}

float GameTimer::TicksSinceReference() const { return SDL_GetTicks() - referenceTime; }

float GameTimer::UpdateTimes()
{
    ++frameCount;

    deltaTime          = (unstoppableDeltaTime) * timeScale;
    time               += deltaTime;

    unscaledDeltaTime  = unstoppableDeltaTime;
    unscaledTime       = TicksSinceReference();

    return deltaTime;
}