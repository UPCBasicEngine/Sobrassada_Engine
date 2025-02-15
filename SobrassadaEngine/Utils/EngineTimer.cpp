#include "EngineTimer.h"
#include "Application.h"
#include "Globals.h"
#include "SDL.h"

EngineTimer::EngineTimer() : time(0), startTime(0), deltaTime(0), isEnabled(false) {}

EngineTimer::~EngineTimer() {}

void EngineTimer::Start()
{
    startTime = SDL_GetTicks();
    isEnabled = true;
}

float EngineTimer::Tick()
{
    deltaTime = TicksSinceStartup() - time;
    time      = TicksSinceStartup();
    return deltaTime;
}

float EngineTimer::TicksSinceStartup() const { return SDL_GetTicks() - startTime; }