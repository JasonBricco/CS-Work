// shared.h
// jmbricco
// CS3411 Assignment 2

// Enables debug printing. When 0, no debug output occurs.
#define ENABLE_DEBUG_PRINT 0

#define MAGIC 0x63746172
#define HEADER_FILE_COUNT 8

#define PRINT(dest, ...) { \
	char buf[256]; \
	sprintf(buf, __VA_ARGS__); \
	write(dest, buf, strlen(buf)); \
}

// Debug helper. 
#if ENABLE_DEBUG_PRINT
#define DEBUG_PRINT(...) PRINT(1, __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

typedef struct
{
	int magic;
	int eop;
	int block_count;
	int file_size[8];
	char deleted[8];
	int file_name[8];
	int next;
} hdr;

// Checks if the given file is an archive file by checking
// for a matching magic number in the file's header.
bool IsArchive(int file)
{
	hdr header;
	
	if (read(file, (uint8_t*)&header, sizeof(header)) < 0)
		return false;

	// Return to the file start after the read
	// since the read moved the file pointer.
	lseek(file, 0, SEEK_SET);

	// Note: I only check the magic here and not EOP. The reason is
	// that only the last header being used points to the end of the file.
	// There's no reason for other headers to point there as they won't
	// be used for figuring out where to write next - they're already full.
	return header.magic == MAGIC;
}

char* ReadFileName(int archive)
{
	char* name = NULL;
	uint16_t size;

	if (read(archive, &size, sizeof(uint16_t)) >= 0)
	{
		name = (char*)malloc(size);
	
		if (read(archive, name, size) < 0)
		{
			PRINT(2, "Failed to read the file name. %s\n", strerror(errno));
			free(name);
		}
	}
	else PRINT(2, "Failed to read the file name size. %s\n", strerror(errno));

	return name;
}

static void PrintHeader(hdr header)
{
	DEBUG_PRINT("HEADER INFORMATION\n");

	DEBUG_PRINT("    Magic: %x\n", header.magic);
	DEBUG_PRINT("    EOP: %d\n", header.eop);
	DEBUG_PRINT("    Block Count: %d\n", header.block_count);
	DEBUG_PRINT("    File Sizes:\n    ");

	for (int i = 0; i < HEADER_FILE_COUNT; ++i)
		DEBUG_PRINT("%d ", header.file_size[i]);

	DEBUG_PRINT("\nDeleted:\n    ");

	for (int i = 0; i < HEADER_FILE_COUNT; ++i)
		DEBUG_PRINT("%d ", header.deleted[i]);

	DEBUG_PRINT("\nFile Names:\n    ");

	for (int i = 0; i < HEADER_FILE_COUNT; ++i)
		DEBUG_PRINT("%d ", header.file_name[i]);

	DEBUG_PRINT("\n    Next: %d\n", header.next);
}
