#pragma once
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <stdio.h>

#define USE_ABSOLUTE_PATH 0

class Device
{
public:
	virtual void RunCommand(std::vector<char*> args) {}
	virtual void DoTick(uint16_t count) {}

	bool StringEquals(char* str, const char* cmd)
	{
		return strcmp(str, cmd) == 0;
	}

	int32_t HexToInt(char* hex)
	{
		return (int32_t)strtol(hex, NULL, 16);
	}

	int32_t FloorToNearest(int32_t value, int32_t multiple)
	{
		return (int32_t)(floorf(value / (float)multiple) * multiple);
	}
};
