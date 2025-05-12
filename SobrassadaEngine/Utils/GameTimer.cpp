#include "GameTimer.h"

#include "Application.h"
#include "Globals.h"

#include "SDL.h"

GameTimer::GameTimer()
    : EngineTimer(), isPaused(false), frameCount(0), unscaledTime(0), unscaledDeltaTime(0), timeScale(1),
      referenceTime(0), unstoppableTime(0), unstoppableDeltaTime(0)
{
}

GameTimer::~GameTimer()
{
}

void GameTimer::Start()
{
    if (isPaused)
    {
        TogglePause();
    }
    else
    {
        Reset();
        EngineTimer::Start();
        referenceTime   = startTime;
        unstoppableTime = startTime;
    }
}

float GameTimer::Tick()
{
    float delta = 0;

    if (isEnabled)
    {
        unstoppableDeltaTime = TicksSinceStartup() - unstoppableTime;
        unstoppableTime      = TicksSinceStartup();

        if (!isPaused)
        {
            ++frameCount;

            deltaTime          = (TicksSinceReference() - unscaledTime) * timeScale;
            time              += deltaTime;

            unscaledDeltaTime  = unstoppableDeltaTime;
            unscaledTime       = TicksSinceReference();
        }
        else
        {
            deltaTime      = 0.0f;
            referenceTime += unstoppableDeltaTime;
        }
    }

    return delta;
}

void GameTimer::TogglePause()
{
    if (isEnabled)
    {
        isPaused = !isPaused;
        if (isPaused) deltaTime = 0.0f;
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
    isPaused             = false;
    isEnabled            = false;

    unstoppableTime      = 0;
    unstoppableDeltaTime = 0;
}

float GameTimer::Step()
{
    if (isEnabled && isPaused)
    {
        ++frameCount;

        deltaTime          = unstoppableDeltaTime * timeScale;
        time              += deltaTime;

        unscaledDeltaTime  = unstoppableDeltaTime;
        unscaledTime      += unscaledDeltaTime;

        referenceTime     -= unscaledDeltaTime;

        return deltaTime;
    }
    else return 0;
}

float GameTimer::TicksSinceReference() const
{
    return SDL_GetTicks() - referenceTime;
}