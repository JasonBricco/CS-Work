#pragma once
#include "device.h"
#include <unordered_map>

#define MAX_LINE_LENGTH 256

std::vector<char*> Split(char* str, char delim);

class Parser
{
	std::unordered_map<std::string, Device*> devices; 

public:
	Parser(Device* clock, Device* cpu, Device* memory, Device* iMemory, Device* cache, Device* iodev)
	{
		devices.insert(std::make_pair("clock", clock));
		devices.insert(std::make_pair("cpu", cpu));
		devices.insert(std::make_pair("memory", memory));
		devices.insert(std::make_pair("imemory", iMemory));
		devices.insert(std::make_pair("cache", cache));
		devices.insert(std::make_pair("iodev", iodev));
	}

	void ParseFile(FILE* file);
};
