// utar.c 
// jmbricco
// CS3411 Assignment 2

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "shared.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		PRINT(2, "Usage: utar <archive>\n");
		return -1;
	}

	int archive = open(argv[1], O_RDWR, 0644);

	if (archive < 0)
	{
		PRINT(2, "Failed to open the archive file. %s\n", strerror(errno));
		return -2;
	}

	if (!IsArchive(archive))
	{
		PRINT(2, "Invalid archive file!\n");
		return -3;
	}

	bool finished = false;

	// Loop as long as we have files to extract.
	while (!finished)
	{
		hdr header;

		// The headers contain the files we need to load. Read the header
		// at this location.
		if (read(archive, (uint8_t*)&header, sizeof(header)) < 0)
		{
			PRINT(2, "Failed to read archive file header.\n");
			return -4;
		}

		PrintHeader(header);

		for (int i = 0; i < HEADER_FILE_COUNT; ++i)
		{
			// If the file is marked as deleted, we skip it.
			if (header.deleted[i] == 1)
				continue;

			if (lseek(archive, header.file_name[i], SEEK_SET) < 0)
				PRINT(2, "Failed to seek to the file name at index %d. %s\n", i, strerror(errno));

			char* name = ReadFileName(archive);
			
			if (name == NULL)
				return -5;

			// Create a new file using the name stored in the archive. 
			int file = open(name, O_CREAT | O_EXCL | O_WRONLY, 0644);

			if (file < 0)
			{
				if (errno == EEXIST) { PRINT(2, "File %s already exists. Move away that file and restart.\n", name); }
				else PRINT(2, "Failed to create the file %s. %s\n", name, strerror(errno));

				return -6;
			}

			// Allocate space to store all bytes in the file in memory. 
			// Then we can write those bytes into the file we just created.
			int fileSize = header.file_size[i];
			uint8_t* bytes = (uint8_t*)malloc(fileSize);

			if (read(archive, bytes, fileSize) < 0)
			{
				PRINT(2, "Failed to read bytes from the file %s. %s\n", name, strerror(errno));
				return -7;
			}

			if (write(file, bytes, fileSize) < 0)
			{
				PRINT(2, "Failed to write the bytes from the archive file to file %s. %s\n", name, strerror(errno));
				return -8;
			}

			if (close(file) != 0)
			{
				PRINT(2, "Failed to close the file %s. %s\n", name, strerror(errno));
				return -9;
			}

			PRINT(1, "File %s was extracted\n", name);

			// Free used memory in case this is a very large file.
			// I don't free the memory in the error cases since the
			// program's termination will free it.
			free(bytes);
			free(name);
		}

		// If header.next is 0, there are no more headers.
		// That means we've loaded all files.
		if (header.next == 0)
			finished = true;
		else
		{
			// We finished loading from the current header. Move to the
			// next one.
			if (lseek(archive, header.next, SEEK_SET) < 0)
			{
				PRINT(2, "Failed to seek to the next header. %s\n", strerror(errno));
				return -10;
			}
		}
	}
}
