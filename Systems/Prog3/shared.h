// shared.h
// jmbricco
// CS3411 Assignment 3

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define INVALID_BYTE 256
#define DICT_SIZE 16

// Helper print macro for convenience.
#define PRINT(...) { \
	char out_buf[256]; \
	sprintf(out_buf, __VA_ARGS__); \
	write(2, out_buf, strlen(out_buf)); \
}

// Used for buffered character reading. This
// allows for better performance than reading
// every individual character.
typedef struct
{
	char data[256];
	int size;
	int index;
} Buffer;

// Reads a byte from the buffer. The buffer buffers 
// bytes read from file in a batch. Once it runs out
// of bytes to return, it loads a new set from file.
static uint32_t ReadByte(Buffer* buf)
{
	// If we have no characters in the buffer, fill the
	// buffer with characters.
	if (buf->size == 0)
	{
		buf->size = read(0, buf->data, sizeof(buf->data));
		buf->index = 0;
	}

	if (buf->size < 0)
	{
		PRINT("Failed to read characters into the buffer.\n");
		exit(-1);
	}
	else if (buf->size > 0)
	{
		// We have characters buffered. Return one.
		uint8_t c = buf->data[buf->index++];
		--buf->size;

		return c;
	}
	else 
	{
		// Return value of 0 means end of file. 
		// Return a value that no character can be.
		return INVALID_BYTE;
	}
}
