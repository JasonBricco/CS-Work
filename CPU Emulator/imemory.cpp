#include "imemory.h"
#include <cassert>

void IMemory::Create(int32_t num)
{
	memSize = num * sizeof(uint32_t);
	memory = (uint32_t*)malloc(memSize);
}

void IMemory::Reset()
{
	memset(memory, 0, memSize);
}

uint32_t IMemory::Fetch(int32_t index)
{
	return memory[index];
}

int IMemory::GetTiming(Instruction instr)
{
	return timing[instr.id];
}

void IMemory::Dump(int32_t addr, int32_t count)
{
	printf("Addr       ");

	for (int i = 0; i < 8; ++i)
		printf("%-6d", i);

	printf("\n");

	int32_t begin = FloorToNearest(addr, 8);

	printf("0x%04X ", (uint16_t)begin);

	int32_t spaces = (addr - begin) * 6;

	for (int s = 0; s < spaces; ++s)
		printf(" ");

	int remOnLine = 8 - (addr - begin);
	int i = 0;

	while (i < count)
	{
		uint32_t inst = memory[addr + i];
		printf("%05X ", inst);

		if (--remOnLine == 0)
		{
			remOnLine = 8;
			printf("\n0x%04X ", (uint16_t)addr + i);
		}

		++i;
	}

	printf("\n\n");
}

void IMemory::Set(int32_t start, std::vector<uint32_t> instructions)
{
	memcpy(memory + start, instructions.data(), instructions.size() * sizeof(uint32_t));
}

static std::vector<uint32_t> GetInstructionsFromFile(char* path)
{
	#if USE_ABSOLUTE_PATH
	const char* p = "C:\\Users\\jason\\Desktop\\Stuff\\Class\\cs3421_emul\\jmbricco\\Sample1_instructions.txt";

	FILE* file = fopen(p, "r");
	#else
	FILE* file = fopen(path, "r");
	#endif
	
	assert(file != nullptr);

	// Get the file size.
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0L, SEEK_SET);

	std::vector<uint32_t> instructions;
	instructions.reserve(size);

	uint32_t instruction;

	// Read the instructions from the file and store them in our list.
	while (fscanf(file, "%05x", &instruction) == 1)
		instructions.push_back(instruction);

	return instructions;
}

void IMemory::RunCommand(std::vector<char*> args)
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
		// Otherwise, we're setting instructions directly.
		if (StringEquals(args[3], "file"))
		{
			std::vector<uint32_t> instructions = GetInstructionsFromFile(args[4]);
			Set(start, std::move(instructions));
		}
		else
		{
			std::vector<uint32_t> instructions;
			instructions.reserve(1000);

			int32_t count = HexToInt(args[3]);

			for (int i = 0; i < count; ++i)
			{
				uint32_t inst = HexToInt(args[i + 4]);
				instructions.push_back(inst);
			}

			Set(start, std::move(instructions));
		}
	}
}
