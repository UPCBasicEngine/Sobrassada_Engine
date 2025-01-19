#pragma once

#include "Globals.h"

class UPCTimer
{
public:
	UPCTimer();
	~UPCTimer();

	float Tick();
private:
	uint32_t previousTicks = 0;
};

