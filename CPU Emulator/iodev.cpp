#include "iodev.h"
#include "parser.h"
#include <assert.h>

void IODevice::Reset()
{
	reg = 0;
}

void IODevice::Load(char* path)
{
	FILE* file = fopen(path, "r");
	assert(file != nullptr);

	int i = 0;
	char line[MAX_LINE_LENGTH];

	while (fgets(line, MAX_LINE_LENGTH, file) != NULL)
	{
		line[strcspn(line, "\r\n")] = '\0';
		std::vector<char*> parts = Split(line, ' ');

		IOTask task = {};
		task.clock = atoi(parts[0]);
		task.read = StringEquals(parts[1], "read");
		task.addr = HexToInt(parts[2]);

		if (!task.read)
			task.value = HexToInt(parts[3]);

		tasks[i++] = task;
	}

	taskCount = i;
}

void IODevice::RunTask(IOTask& task)
{
	if (task.read)
		reg = memory->GetByte(task.addr);
	else // Do a write if read is false.
	{
		memory->SetByte(task.addr, task.value);
		cache->SetByte(0xFF, 0);
	}
}

void IODevice::DoTick(uint16_t count)
{
	if (pendingTask == taskCount)
		return;

	IOTask& task = tasks[pendingTask];

	switch (state)
	{
		case TICK_WAIT:
		{
			if (waitingForTick == count)
			{
				RunTask(task);
				++pendingTask;
				state = TASK_WAIT;
			}
		}

		case TASK_WAIT:
		{
			if (task.clock == count)
			{
				state = TICK_WAIT;
				waitingForTick = count + memory->GetWait();
			}
		} break;
	}
}

void IODevice::Dump()
{
	printf("IO Device: 0x%02X\n\n", reg);
}

void IODevice::RunCommand(std::vector<char*> args)
{
	if (StringEquals(args[1], "reset"))
		Reset();
	else if (StringEquals(args[1], "load"))
		Load(args[2]);
	else if (StringEquals(args[1], "dump"))
		Dump();
}
