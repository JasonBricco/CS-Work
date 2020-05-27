#pragma once
#include "device.h"

struct MemorySource : public Device
{
	virtual uint8_t GetByte(int32_t addr) = 0;
	virtual void SetByte(int32_t addr, uint8_t byte) = 0;
	virtual int GetWait() = 0;
};

class DataMemory : public MemorySource
{
	int32_t memSize;
	uint8_t* memory;

	void Create(int32_t size);
	void Reset();
	void Set(int32_t start, std::vector<uint8_t> bytes);
	void Dump(int32_t addr, int32_t bytes);

public:
	uint8_t GetByte(int32_t addr) override;
	void SetByte(int32_t addr, uint8_t byte) override;
	void RunCommand(std::vector<char*> args) override;

	inline int GetWait() override
	{
		return 4;
	}
};
