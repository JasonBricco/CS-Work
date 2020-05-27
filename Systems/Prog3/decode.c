// decode.c 
// jmbricco
// CS3411 Assignment 3

#include "shared.h"

typedef struct
{
	uint8_t byte;
	int count;
} BitReader;

// Reads a bit from the buffer. One byte is read from
// file and entered into the BitReader. Then each bit
// can be extracted from that byte.
static uint32_t ReadBit(BitReader* reader, Buffer* buf)
{
	if (reader->count == 0)
	{
		uint32_t byte = ReadByte(buf);

		if (byte == INVALID_BYTE)
			return INVALID_BYTE;

		reader->byte = (uint8_t)byte;
		reader->count = 8;
	}

	uint32_t bit = (reader->byte >> (reader->count - 1)) & 1;
	--reader->count;

	return bit;
}

// Write all data in the buffer out to file.
static void FlushBuffer(Buffer* buf)
{
	if (buf->index != 0)
	{
		// buf->index is being used as the size, since it counts
		// how many characters are in this buffer. We can use it 
		// directly, since each character is 1 byte.
		if (write(1, buf, buf->index) < 0)
		{
			PRINT("Failed to write the byte to the buffer.\n");
			exit(-1);
		}

		buf->index = 0;
	}
}

// Write a byte into the buffer. The buffer will store
// bytes until its capacity is filled, and then flush 
// them to file.
static void WriteByte(Buffer* buf, uint8_t byte) 
{       
	buf->data[buf->index++] = byte;             

	if (buf->index == sizeof(buf->data)) 
		FlushBuffer(buf); 
}

// Sets the bit 'bit' into 'value' at 'pos' position.
static uint32_t SetBit(uint32_t value, uint32_t bit, int pos)
{
    return bit == 1 ? (uint32_t)(value | (1 << pos)) : value;
}

int main()
{
	char dictionary[DICT_SIZE] = {0};

	// Read dictionary from the file first.
	if (read(0, dictionary + 1, DICT_SIZE - 1) <= 0)
	{
		PRINT("Failed to read the dictionary from the file. %s\n", strerror(errno));
		return -1;
	}

	BitReader reader = { 0, 0 };
	Buffer readBuffer = {0};
	Buffer writeBuffer = {0};

	// Decoding loop.
	while (1)
	{
		uint32_t bit = ReadBit(&reader, &readBuffer);

		if (bit == INVALID_BYTE)
		{
			PRINT("Failed to read the bit (received INVALID_BYTE)\n");
			return -1;
		}

		if (bit == 0)
		{
			// The first bit is 0. This means the next 8 bits is the
			// character we must read out directly.

			uint32_t c = 0;

			for (int i = 0; i < 8; ++i)
			{
				uint32_t bit = ReadBit(&reader, &readBuffer);
				c = SetBit(c, bit, i);
			}

			WriteByte(&writeBuffer, (uint8_t)c);
		}
		else
		{
			if (bit != 1)
			{
				PRINT("Invalid bit found!\n");
				return -1;
			}

			bit = ReadBit(&reader, &readBuffer);

			if (bit == 0)
			{
				// We see '10', which is the encoding for the null byte.
				WriteByte(&writeBuffer, '\0');
			}
			else
			{
				// We see '11'. We must read the run length and offset.
				// This character is in our dictionary.
				uint32_t runLength = 0;
				uint32_t offset = 0;

				uint32_t run0 = ReadBit(&reader, &readBuffer);
				uint32_t run1 = ReadBit(&reader, &readBuffer);

				uint32_t off0 = ReadBit(&reader, &readBuffer);
				uint32_t off1 = ReadBit(&reader, &readBuffer);
				uint32_t off2 = ReadBit(&reader, &readBuffer);
				uint32_t off3 = ReadBit(&reader, &readBuffer); 

				runLength = SetBit(runLength, run0, 0);
				runLength = SetBit(runLength, run1, 1);

				offset = SetBit(offset, off0, 0);
				offset = SetBit(offset, off1, 1);
				offset = SetBit(offset, off2, 2);
				offset = SetBit(offset, off3, 3);

				// run length and offset being 0 signifies the
				// end of file encoding, so end here.
				if (runLength == 0 && offset == 0)
					break;

				// Use offset to find the symbol in the dictionary
				// to output to file.
				char symbol = dictionary[offset];

				for (int i = 0; i < runLength + 1; ++i)
					WriteByte(&writeBuffer, symbol); 
			}
		}

		// If the buffer was not filled, it won't be output to the file.
		// Ensure it is always output here.
		FlushBuffer(&writeBuffer);
	}
}
