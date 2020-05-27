// Jason Bricco (jmbricco)
// CS3411 
// Assignment 1

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define  F_first        1   /* This is the first call to the function. */
#define  F_last         2   /* This is the last call to the function. Free the memory area. */
#define  F_data_int     3   /* Void * argument points to integer data. */
#define  F_data_char    4   /* Void * argument points to character string. */
#define  F_data_float   5   /* Void * argument points to a float data value. */
#define  F_print        6   /* Print the accumulated values. */

void* f(int32_t code, void* mem, void* data)
{
	if (code == F_first)
	{
		// Size is passed in using the pointer address itself, so 
		// cast it to an integer directly.
		intptr_t size = (intptr_t)data;

		if (mem != NULL)
		{
			printf("Error: memory must be NULL on the first call.\n");
			return NULL;
		}

		if (size == 0)
			return NULL;

		// Allocate size bytes worth of memory and interpret it as a
		// pointer to 2 byte values. This allows us to set the beginning
		// value to 2 (representing the next free point to write to).
		uint16_t* memory = (uint16_t*)malloc(size);
		*memory = 2;

		return memory;
	}
	else if (code == F_last)
	{
		// Deallocate memory if it exists and set it to NULL for safety.
		if (mem != NULL)
		{
			free(mem);
			mem = NULL;
		}
		else printf("Cannot free the memory; it's already null.\n");
	}
	else if (code == F_print)
	{
		if (mem != NULL)
		{
			uint16_t size = *((uint16_t*)mem);
			uint8_t* end = (uint8_t*)mem + size;
			uint8_t* ptr = (uint8_t*)mem + 2;

			// Once ptr equals the end pointer, we've gone through all assigned memory.
			while (ptr != end)
			{
				// The type always comes first, so we expect to find a type initially.
				uint8_t type = *ptr++;

				if (type == F_data_int)
				{
					// Our integer was written in big endian format. 
					// We can use an intrinsic to quickly swap it to little endian
					// for printing. 
					int32_t i = __builtin_bswap32(*((int32_t*)ptr));
					printf("%d ", i);
					ptr += sizeof(int32_t);
				} 
				else if (type == F_data_char)
				{
					// Read the string out until the null terminating character is reached.
					// strlen will give the length not counting the null terminator.
					// I add 1 to advance past the null terminator as well.
					char* str = (char*)ptr;
					printf("%s", str);
					ptr += strlen(str) + 1;
				}
				else if (type == F_data_float)
				{
					float flt;

					// View the bytes of the flt stack variable
					// so that we can assign individual bytes.
					uint8_t* fPtr = (uint8_t*)&flt;

					// I know of no built in byte swap for floats, and so I manually
					// read out the bytes in reverse order to construct a float in
					// little endian.
					for (int i = 3; i >= 0; --i)
						fPtr[i] = *ptr++;

					printf("%.1f ", flt);
				}
				else printf("Error: unknown data type (%d)\n", type);
			}
		}
		else printf("Cannot print values, memory is null\n");
	}
	else
	{
		if (mem != NULL)
		{
			// Get a pointer to the start as uint16_t so the
			// size of assigned memory can be adjusted.
			uint16_t* start = (uint16_t*)mem;

			// Pointer to the free area we can write to. 
			// Write the code first.
			uint8_t* ptr = (uint8_t*)mem + *start;
			*ptr++ = (uint8_t)code;

			// View the data as a pointer to bytes.
			uint8_t* dataPtr = (uint8_t*)data;

			switch (code)
			{
				case F_data_int:
				{
					// Write the integer bytes in big endian format
					// by placing the more significant bytes first.
					for (int i = 3; i >= 0; --i)
						*ptr++ = dataPtr[i];

					// Add the size of the integer we wrote + the size of the code.
					(*start) += sizeof(int32_t) + 1;
				} break;

				case F_data_char:
				{
					// strcpy copies all character up to and including the null
					// terminator, which is what we want. 
					strcpy((char*)ptr, (char*)data);

					// Add the size of the string + the null terminator + the code.
					(*start) += strlen((char*)data) + 2;
				} break;

				case F_data_float:
				{
					// Write the float bytes in big endian format. 
					for (int i = 3; i >= 0; --i)
					*ptr++ = dataPtr[i];
					
					// Add the size of the float + the code.
					(*start) += sizeof(float) + 1;
				} break;

				default:
					printf("Error: invalid code.\n");
					break;
			}
		}
		else printf("Cannot add data, memory is null.\n");
	}
}
