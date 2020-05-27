#pragma once
#include "device.h"
#include "memory.h"
#include "imemory.h"

#define REGISTER_COUNT 8

class CPU : public Device
{
	uint16_t waitingForTick;

	bool halted;

	uint8_t registers[REGISTER_COUNT];
	uint8_t PC;
	uint16_t TC;

	MemorySource* source;
	IMemory* instructions;

	void Reset();
	void SetReg(char* reg, uint8_t value);
	void Dump();

	void RunInstruction(Instruction inst);
	void LoadWord(Instruction i);
	void StoreWord(Instruction i);
	void Add(Instruction i);
	void AddImmediate(Instruction i);
	void Multiply(Instruction i);
	void Invert(Instruction i);
	void BranchIfEqual(Instruction i);
	void Halt();

	void DoTick(uint16_t count) override;

public:
	CPU(IMemory* instructions, MemorySource* source)
	{
		this->instructions = instructions;
		this->source = source;
		waitingForTick = 0;
	}

	void RunCommand(std::vector<char*> args) override;
};
