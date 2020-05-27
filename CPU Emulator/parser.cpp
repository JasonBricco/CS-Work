#include "parser.h"
#include <cstring> 
#include <vector>

std::vector<char*> Split(char* str, char delim)
{
    char delims[2] = {};
    delims[0] = delim;

    char* part;

    std::vector<char*> parts;
    part = strtok(str, delims);

    while (part != nullptr)
    {
        parts.push_back(part);
        part = strtok(nullptr, delims);
    }

    return parts;
}

void Parser::ParseFile(FILE* file)
{
	char line[MAX_LINE_LENGTH];

	while (fgets(line, MAX_LINE_LENGTH, file) != NULL)
	{
		// fgets() returns the line including the newline character \n. strcspn() allows us
		// to remove the unwanted ending characters. It returns the size of the portion of 
		// the string that doesn't include the \r or \n. At this position we insert a null
		// terminator to end the string early.
		line[strcspn(line, "\r\n")] = '\0';

		std::vector<char*> parts = Split(line, ' ');

		std::string deviceName(parts[0]);
		Device* device = devices.find(deviceName)->second;

		device->RunCommand(std::move(parts));
	}
}
