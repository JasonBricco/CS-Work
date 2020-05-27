#pragma once
#include "device.h"
#include "instruction.h"

class IMemory : public Device
{
	uint32_t* memory;

	int timing[MAX_INSTRUCTIONS] = { 1, 1, 2, 1, 1, 1, 1, 1 };

	int32_t memSize;

	void Create(int32_t num);
	void Reset();
	void Dump(int32_t addr, int32_t count);
	void Set(int32_t start, std::vector<uint32_t> instructions);

public:
	uint32_t Fetch(int32_t index);
	int GetTiming(Instruction instr);
	void RunCommand(std::vector<char*> args) override;
};
