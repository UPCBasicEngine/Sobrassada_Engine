#pragma once

#include "Globals.h"

class UPCTimer
{
public:
	
	UPCTimer();
	~UPCTimer();

public:

	float Tick();

private:
	uint32_t previousTicks = 0;
};

