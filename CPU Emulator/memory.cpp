#include "memory.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cassert>

void DataMemory::Create(int32_t size)
{
	memory = (uint8_t*)malloc(size);
	memSize = size;
}

void DataMemory::Reset()
{
	memset(memory, 0, memSize);
}

void DataMemory::Dump(int32_t addr, int32_t bytes)
{
	printf("Addr   ");

	for (int i = 0; i < 16; ++i)
		printf("%02X ", i);

	printf("\n");

	uint8_t* start = memory + addr;
	int32_t prevAddr = addr;

	// Add 16 and then round it down to the nearest multiple of 16 to get the
	// next starting address.
	addr = FloorToNearest(addr, 16);

	// Get the number of spaces to print.
	int spaceCount = prevAddr - addr;
	int32_t printCount = 16 - spaceCount;

	printf("0x%04X ", (uint16_t)addr);

	for (int s = 0; s < spaceCount; ++s)
		printf("   ");

	int i = 0;

	while (i < bytes)
	{
		for (int rowP = 0; rowP < printCount; ++rowP)
		{
			uint8_t byte = *(start + i);
			printf("%02X ", byte);

			if (++i == bytes)
			{
				printf("\n\n");
				return;
			}
		}

		printCount = 16;
		addr = FloorToNearest(addr + 16, 16);

		printf("\n");
		printf("0x%04X ", (uint16_t)addr);
	}
}

void DataMemory::Set(int32_t start, std::vector<uint8_t> bytes)
{
	uint8_t* src = memory + start;
	memcpy(src, bytes.data(), bytes.size());
}

static std::vector<uint8_t> GetBytesFromFile(char* path)
{
	FILE* file = fopen(path, "r");
	assert(file != nullptr);

	// Get the file size.
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0L, SEEK_SET);

	std::vector<uint8_t> bytes;
	bytes.reserve(size);

	int32_t byte;

	// Read the bytes from the file and store them in our list.
	while (fscanf(file, "%x", &byte) == 1)
		bytes.push_back((uint8_t)byte);

	return bytes;
}

uint8_t DataMemory::GetByte(int32_t addr)
{
	return *(memory + addr);
}

void DataMemory::SetByte(int32_t addr, uint8_t byte)
{
	*(memory + addr) = byte;
}

void DataMemory::RunCommand(std::vector<char*> args)
{
	if (StringEquals(args[1], "create"))
	{
		int32_t size = HexToInt(args[2]);
		Create(size);
	}
	else if (StringEquals(args[1], "reset"))
		Reset();
	else if (StringEquals(args[1], "dump"))
	{
		int32_t start = HexToInt(args[2]);
		int32_t count = HexToInt(args[3]);

		Dump(start, count);
	}
	else if (StringEquals(args[1], "set"))
	{
		int32_t start = HexToInt(args[2]);

		// If the third argument is "file", then we're trying to load a file.
		// Otherwise, we're setting bytes directly.
		if (StringEquals(args[3], "file"))
		{
			std::vector<uint8_t> bytes = GetBytesFromFile(args[4]);
			Set(start, std::move(bytes));
		}
		else
		{
			std::vector<uint8_t> bytes;
			bytes.reserve(4096);

			int32_t count = HexToInt(args[3]);

			for (int i = 0; i < count; ++i)
				bytes.push_back((uint8_t)HexToInt(args[i + 4]));

			Set(start, std::move(bytes));
		}
	}
}
