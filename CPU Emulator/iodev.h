#pragma once
#include "device.h"
#include "memory.h"

#define MAX_IO_TASKS 100

struct IOTask
{
	int clock;
	bool read;
	int32_t addr;
	int32_t value;
};

enum IOState
{
	TASK_WAIT,
	TICK_WAIT
};

class IODevice : public Device
{
	// The device's register.
	uint8_t reg;

	int pendingTask, taskCount;
	IOTask tasks[MAX_IO_TASKS];

	IOState state;
	int waitingForTick;

	MemorySource* memory;
	MemorySource* cache;

	void RunTask(IOTask& task);

	void Reset();
	void Load(char* path);

	void Dump();

	void DoTick(uint16_t count) override;

public:
	IODevice(MemorySource* memory, MemorySource* cache)
	{
		this->memory = memory;
		this->cache = cache;
		reg = 0;
		pendingTask = 0;
		state = TASK_WAIT;
	}

	void RunCommand(std::vector<char*> args) override;
};
