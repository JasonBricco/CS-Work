#include "clock.h"
#include <cstdlib>
#include <cstdio>

void Clock::RegisterForTicks(Device* device)
{
	listeners.push_back(device);
}

void Clock::Reset()
{
	count = 0;
}

void Clock::Tick(uint16_t amt)
{
	for (uint16_t i = 0; i < amt; ++i)
	{
		for (Device* listener : listeners)
			listener->DoTick(count);

		++count;
	}
}

void Clock::Dump()
{
	printf("Clock: %hu\n\n", count);
}

void Clock::RunCommand(std::vector<char*> args)
{
	if (StringEquals(args[1], "reset"))
		Reset();
	else if (StringEquals(args[1], "tick"))
	{
		int amt = atoi(args[2]);
		Tick((uint16_t)amt);
	}
	else if (StringEquals(args[1], "dump"))
		Dump();
}
