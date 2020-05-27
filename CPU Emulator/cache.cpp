#include "cache.h"

uint8_t Cache::GetByte(int32_t addr)
{
	missed = false;

	if (!enabled)
	{
		missed = true;
		return source->GetByte(addr);
	}

	if (addr == CACHE_INVALIDATE)
	{
		memset(valid, 0, sizeof(valid));
		return 0;
	}

	int line = addr / CACHE_SIZE;
	int srcStart = line * CACHE_SIZE;
	int index = addr & (CACHE_SIZE - 1);

	bool anyValidData = false;

	for (int i = 0; i < CACHE_SIZE; ++i)
	{
		if (valid[i])
		{
			anyValidData = true;
			break;
		}
	}

	if (line != clo || !anyValidData)
	{
		Flush();

		for (int i = 0; i < CACHE_SIZE; ++i)
		{
			memory[i] = source->GetByte(srcStart + i);
			valid[i] = true;
		}
		
		clo = line;
		missed = true;
	}
	else
	{
		bool isValid = valid[index];

		if (isValid || (!isValid && dataWritten[index]))
			return memory[index];
		else
		{
			memory[index] = source->GetByte(srcStart + index);
			valid[index] = true;
			missed = true;
		}
	}

	return memory[index];
}

void Cache::Write(int addr, uint8_t byte)
{
	int index = addr & (CACHE_SIZE - 1);
	memory[index] = byte;
	dataWritten[index] = true;
	valid[index] = false;
}

bool Cache::Flush()
{
	bool success = false;
	int startAddr = clo * CACHE_SIZE;

	for (int i = 0; i < CACHE_SIZE; ++i)
	{
		if (dataWritten[i])
		{
			source->SetByte(startAddr + i, memory[i]);
			dataWritten[i] = false;
			valid[i] = true;
			success = true;
		}
	}

	return success;
}

void Cache::SetByte(int32_t addr, uint8_t byte)
{
	if (!enabled)
	{
		source->SetByte(addr, byte);
		missed = true;
		return;
	}

	if (addr == CACHE_INVALIDATE)
	{
		missed = Flush();
		return;
	}

	int line = addr / CACHE_SIZE;
	missed = line != clo;

	if (!missed)
		Write(addr, byte);
	else
	{
		bool success = Flush();
		clo = line;
		memset(valid, 0, sizeof(valid));
		Write(addr, byte);

		// Hacky... this is because I used the missed value to figure out timing.
		if (!success)
			missed = false;
	}
}

void Cache::Reset()
{
	enabled = false;
	clo = -1;
	memset(valid, 0, sizeof(valid));
	memset(dataWritten, 0, sizeof(dataWritten));
}

void Cache::Dump()
{
	printf("CLO        : 0x%02X\n", clo == -1 ? 0 : clo);
	printf("cache data :");

	for (int i = 0; i < CACHE_SIZE; ++i)
		printf(" 0x%02X", memory[i]);

	printf("\nFlags      :   ");

	for (int i = 0; i < CACHE_SIZE; ++i)
	{
		if (!valid[i])
		{
			if (dataWritten[i])
				printf("W    ");
			else printf("I    ");
		}
		else printf("V    ");
	}

	printf("\n\n");
}

void Cache::RunCommand(std::vector<char*> args)
{
	if (StringEquals(args[1], "reset"))
		Reset();
	else if (StringEquals(args[1], "on"))
		enabled = true;
	else if (StringEquals(args[1], "off"))
	{
		Flush();
		enabled = false;
	}
	else if (StringEquals(args[1], "dump"))
		Dump();
}
