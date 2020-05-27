#include "parser.h"
#include "clock.h"
#include "cpu.h"
#include "memory.h"
#include "cache.h"
#include "iodev.h"
#include <cstdio>

int main(int argc, char** argv)
{
	#if USE_ABSOLUTE_PATH
	const char* path = "C:\\Users\\jason\\Desktop\\Stuff\\Class\\cs3421_emul\\jmbricco\\Sample1_input.txt";
	#else
	char* path = argv[1];
	#endif

	FILE* file = fopen(path, "r");

	if (file == nullptr)
	{
		printf("ERROR: No file was specified!\n");
		return -1;
	}

	Clock clock;
	IMemory iMemory;
	DataMemory memory;
	Cache cache(&memory);
	CPU cpu(&iMemory, &cache);
	IODevice iodev(&memory, &cache);

	clock.RegisterForTicks(&cpu);
	clock.RegisterForTicks(&iodev);

	Parser parser(&clock, &cpu, &memory, &iMemory, &cache, &iodev);
	parser.ParseFile(file);
}
