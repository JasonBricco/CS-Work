#pragma once
#include "device.h"

class Clock : public Device
{
	uint16_t count;
	std::vector<Device*> listeners;

	void Reset();
	void Tick(uint16_t amt);
	void Dump();

public:
	void RegisterForTicks(Device* device);
	void RunCommand(std::vector<char*> args) override;
};
