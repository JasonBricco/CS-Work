#pragma once
#include "memory.h"

#define CACHE_SIZE 8
#define CACHE_INVALIDATE 0xFF

class Cache : public MemorySource
{
	MemorySource* source;

	uint8_t memory[CACHE_SIZE];
	bool dataWritten[CACHE_SIZE];
	bool valid[CACHE_SIZE];

	int8_t clo;

	bool enabled;
	bool missed;

	bool Flush();
	void Write(int addr, uint8_t byte);
	void Reset();
	void Dump();

public:
	Cache(MemorySource* source)
	{
		this->source = source;
		clo = -1;
	}

	inline int GetWait() override
	{
		return missed ? 4 : 0;
	}

	uint8_t GetByte(int32_t addr) override;
	void SetByte(int32_t addr, uint8_t byte) override;
	void RunCommand(std::vector<char*> args) override;
};
