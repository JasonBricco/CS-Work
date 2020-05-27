// ctar.c 
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

#define MODE_APPEND 0
#define MODE_DELETE 1

// Checks to see if the user passed -a or -d, to 
// determine whether we are using append or delete mode.
static int GetMode(char* option)
{
	if (strcmp(option, "-a") == 0)
		return MODE_APPEND;
	else if (strcmp(option, "-d") == 0)
		return MODE_DELETE;
	else return -1;
}

// Write the given header to the specified file, at the 
// current file pointer.
static void WriteHeader(int file, hdr header)
{
	if (write(file, (uint8_t*)&header, sizeof(header)) < 0)
		PRINT(2, "Failed to write the header file into the file. %s\n", strerror(errno));
}

// Creates a header and writes it into the file.
static hdr CreateHeader(int loc, int file)
{
	hdr header = {0};
	header.magic = MAGIC;
	header.eop = loc + sizeof(header);

	// No files are initially present.
	for (int i = 0; i < HEADER_FILE_COUNT; ++i)
		header.deleted[i] = 1;

	WriteHeader(file, header);
	return header;
}

// Returns true if this file is in the header already.
static bool FileInHeader(int archive, hdr header, char* file)
{
	bool found = false;

	// Go through each file referenced by this header, 
	// read out the name, and check if it matches the specified file.
	for (int i = 0; i < HEADER_FILE_COUNT && !found; ++i)
	{
		int nameLoc = header.file_name[i];

		if (lseek(archive, nameLoc, SEEK_SET) < 0)
			PRINT(2, "FileInHeader: failed to seek. %s\n", strerror(errno));

		char* name = ReadFileName(archive);

		if (strcmp(name, file) == 0)
			found = true;

		free(name);
	}

	return found;
}

// Adds the specified file to the archive file.
static bool AddFileToArchive(int archive, char* fileName)
{
	DEBUG_PRINT("Adding file %s to the archive.\n", fileName);

	// File to add to the archive.
	int fileToAdd = open(fileName, O_RDONLY, 0644);

	// If we fail to open the file, we know it is not a valid
	// file that can be added.
	if (fileToAdd < 0)
	{
		PRINT(2, "Invalid file specified: %s\n", fileName);
		return false;
	}

	// Start at the beginning of the archive file to figure out
	// which header to use.
	int headerLoc = lseek(archive, 0, SEEK_SET);

	if (headerLoc < 0)
		PRINT(2, "AddFileToArchive: failed to seek to the file beginning. %s\n", strerror(errno));

	DEBUG_PRINT("Initial header location (should be 0): %d\n", headerLoc);

	hdr header;
	bool foundHeader = false;

	while (!foundHeader)
	{
		// Read the header at this location.
		if (read(archive, (uint8_t*)&header, sizeof(header)) < 0)
		{
			PRINT(2, "Failed to read archive file header. %s\n", strerror(errno));
			return false;
		}

		if (FileInHeader(archive, header, fileName))
		{
			PRINT(2, "That file is already in the archive.\n");
			return false;
		}

		DEBUG_PRINT("Header found at %d with %d files\n", headerLoc, header.block_count);

		// This header is full and we can't add this file to it.
		if (header.block_count == HEADER_FILE_COUNT)
		{
			DEBUG_PRINT("Header is full.\n");

			// next will only be 0 if there is no next header.
			// In this case, we create a new one.
			if (header.next == 0)
			{
				// The new header should be written at the end of the file.
				// Set the next pointer of the current header to it.
				int newHeaderLoc = lseek(archive, 0, SEEK_END);

				if (newHeaderLoc < 0)
					PRINT(2, "AddFileToArchive: failed to seek to the file end. %s\n", strerror(errno));

				header.next = newHeaderLoc;

				// Return to the location of the current header to write it
				// back to disk, since we changed its next pointer. 
				if (lseek(archive, headerLoc, SEEK_SET) < 0)
					PRINT(2, "AddFileToArchive: failed to seek to the header location. %s\n", strerror(errno));

				WriteHeader(archive, header);

				// Set our location to the new header, and create it.
				headerLoc = lseek(archive, newHeaderLoc, SEEK_SET);

				if (headerLoc < 0)
					PRINT(2, "AddFileToArchive: failed to seek to the new header location. %s\n", strerror(errno));

				header = CreateHeader(headerLoc, archive);

				foundHeader = true;

				DEBUG_PRINT("No next header, creating a new one. New location: %d\n", headerLoc);
			}
			else 
			{
				headerLoc = lseek(archive, header.next, SEEK_SET);

				if (headerLoc < 0)
					PRINT(2, "AddFileToArchive: failed to seek to the next header. %s\n", strerror(errno));

				DEBUG_PRINT("New header location: %d\n", headerLoc);
			}
		}
		else foundHeader = true;
	}

	// Add 1 to the size to account for the null terminating character.
	uint16_t fileNameSize = (uint16_t)strlen(fileName) + 1;

	// Move to where we can begin writing and write the file size/name.
	int nameLoc = lseek(archive, header.eop, SEEK_SET);

	if (nameLoc < 0)
		PRINT(2, "AddFileToArchive: failed to seek to the header eop. %s\n", strerror(errno));

	DEBUG_PRINT("Writing the file name to location %d, size = %d\n", nameLoc, fileNameSize);

	if (write(archive, &fileNameSize, sizeof(uint16_t)) < 0)
		PRINT(2, "Failed to write the file name size. %s\n", strerror(errno));

	if (write(archive, fileName, fileNameSize) < 0)
		PRINT(2, "Failed to write the file name. %s\n", strerror(errno));

	struct stat info;

	// Gather information about this file. We want to get the size in bytes.
	if (fstat(fileToAdd, &info) < 0)
		PRINT(2, "Failed to get stats for the file. %s\n", strerror(errno));

	DEBUG_PRINT("Size in bytes of file to add: %d\n", (int)info.st_size);

	// Read the bytes of the file to add in, so we can write them into our archive file.
	void* bytes = malloc(info.st_size);
	
	if (read(fileToAdd, bytes, info.st_size) < 0)
		PRINT(2, "Failed to read the bytes from the file. %s\n", strerror(errno));

	if (write(archive, bytes, info.st_size) < 0)
		PRINT(2, "Failed to write the bytes to the archive. %s\n", strerror(errno));

	free(bytes);

	// Update the header to reference the file added.
	header.file_size[header.block_count] = info.st_size;
	header.file_name[header.block_count] = nameLoc;
	header.deleted[header.block_count] = 0;
	++header.block_count;

	// Set the end of file pointer to the end of the file.
	header.eop = lseek(archive, 0, SEEK_END);

	if (header.eop < 0)
		PRINT(2, "AddFileToArchive: failed to seek to the end of the file. %s\n", strerror(errno));

	// Write the modified header file back into the archive.
	// We must seek back to where our header is located first.
	int writeLoc = lseek(archive, headerLoc, SEEK_SET);

	if (writeLoc < 0)
		PRINT(2, "AddFileToArchive: failed to seek to the headerLoc. %s\n", strerror(errno));

	WriteHeader(archive, header);

	if (close(fileToAdd) != 0)
		PRINT(2, "Failed to close the archive file. %s\n", strerror(errno));

	DEBUG_PRINT("New end of file: %d. Final header: \n", header.eop);
	PrintHeader(header);

	PRINT(1, "File %s added to the archive successfully.\n", fileName);
}

static void DeleteFileFromArchive(int archive, char* fileName)
{
	DEBUG_PRINT("Deleting file %s\n", fileName);

	// Start at the beginning of the archive file to figure out
	// which header has our file to delete.
	int headerLoc = lseek(archive, 0, SEEK_SET);

	if (headerLoc < 0)
		PRINT(2, "DeleteFileFromArchive: failed to seek to the beginning of the file. %s\n", strerror(errno));

	// Loop until either we found the file to delete
	// or run out of headers to check.
	while (1)
	{
		hdr header;

		if (read(archive, (uint8_t*)&header, sizeof(header)) < 0)
		{
			PRINT(2, "Failed to read archive file header for file deletion. %s\n", strerror(errno));
			return;
		}

		for (int i = 0; i < HEADER_FILE_COUNT; ++i)
		{
			int loc = header.file_name[i];

			// 0 means there is no file here. 
			if (loc == 0)
				continue;

			lseek(archive, loc, SEEK_SET);
			char* name = ReadFileName(archive);

			DEBUG_PRINT("Next file: %s\n", name);

			if (strcmp(name, fileName) == 0)
			{
				// Update deleted to 1 (not present) and ensure there's no file name pointer.
				// This way it will be skipped for future deletions if the same file was
				// added multiple times (for convenience during debugging - we don't actually
				// allow the same file to be added more than once).
				header.deleted[i] = 1;
				header.file_name[i] = 0;

				// Write the changes to the header back to disk.
				if (lseek(archive, headerLoc, SEEK_SET) < 0)
					PRINT(2, "DeleteFileFromArchive: failed to seek to the header location. %s\n", strerror(errno));

				WriteHeader(archive, header);

				free(name);
				PRINT(1, "File successfully deleted.\n");

				return;
			}
			else free(name);
		}

		if (header.next == 0)
		{
			PRINT(2, "The file %s does not exist in the archive.\n", fileName);
			return;
		}
		else
		{
			DEBUG_PRINT("Not in this header. Going to the next at %d\n", header.next);
			headerLoc = lseek(archive, header.next, SEEK_SET);

			if (headerLoc < 0)
				PRINT(2, "DeleteFileFromArchive: failed to seek to the next header. %s\n", strerror(errno));
		}
	}
}

int main(int argc, char** argv)
{
	// We must have at least 3 arguments: program name, -a or -d, and an archive name.
	if (argc < 3)
	{
		PRINT(2, "Invalid number of arguments.\n");
		return -1;
	}

	int mode = GetMode(argv[1]);

	if (mode == -1)
	{
		PRINT(2, "Argument 1 must be -a to append or -d to delete\n");
		return -2;
	}

	// The second argument is always the archive file name. If we have
	// three, we assume we are making a new archive file.
	if (argc == 3)
	{
		if (mode != MODE_APPEND)
		{
			// There should never be two arguments with the -d option.
			PRINT(2, "No file provided to delete!\n");
			return -3;
		}

		int file = open(argv[2], O_CREAT | O_EXCL | O_WRONLY, 0644);

		if (file < 0)
		{
			if (errno == EEXIST) { PRINT(2, "A file with that name already exists\n"); }
			else PRINT(2, "Failed to create the archive file. %s\n", strerror(errno));

			return -4;
		}

		// Create the initial header for the file.
		CreateHeader(0, file);

		PRINT(1, "Successfully created the archive file.\n");

		if (close(file) != 0)
			PRINT(2, "Failed to close the archive file. %s\n", strerror(errno));
	}
	else
	{
		DEBUG_PRINT("Opening archive file: %s\n", argv[2]);

		int archive = open(argv[2], O_RDWR, 0644);

		if (archive < 0)
		{
			PRINT(2, "Failed to open the archive file. %s\n", strerror(errno));
			return -5;
		}

		if (!IsArchive(archive))
		{
			PRINT(2, "Argument 2 is not an archive file!\n");
			return -6;
		}

		if (mode == MODE_APPEND)
		{
			for (int i = 3; i < argc; ++i)
				AddFileToArchive(archive, argv[i]);
		}
		else
		{
			// We only support deleting a single file at a time, as per the spec.
			if (argc != 4)
			{
				PRINT(2, "Usage: ctar -d <archive> <file>\n");
				return -7;
			}

			DeleteFileFromArchive(archive, argv[3]);
		}
	}

	return 0;
}
