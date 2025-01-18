#include "UPCTimer.h"

#include "SDL_timer.h"

UPCTimer::UPCTimer()
{
}

UPCTimer::~UPCTimer()
{
}

float UPCTimer::Tick()
{
	uint32_t currentTicks = SDL_GetTicks();
	uint32_t deltaTicks = currentTicks - previousTicks;
	float deltaTime = deltaTicks / 1000.f;

	previousTicks = currentTicks;

	return deltaTime;
}
