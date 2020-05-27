// encode.c 
// jmbricco
// CS3411 Assignment 3

#include "shared.h"

#define SYM_SIZE 256

// Used for writing bits. It buffers bits
// up into a byte and writes the byte to
// disk when it's full.
typedef struct
{
	uint8_t byte;
	int count;
} BitWriter;

// Represents a symbol in the symbol table.
// It stores the character with its frequency,
// allowing for no information to be lost
// when sorting the table.
typedef struct
{
	char c;
	int freq;
} Symbol;

// Comparison function for sorting the symbols table.
// Higher frequency precedes lower frequency.
// In the case of a tie, smaller magnitude precedes
// higher magnitude.
int CompareSymbols(const void* aPtr, const void* bPtr)  
{ 
	Symbol* a = (Symbol*)aPtr;
	Symbol* b = (Symbol*)bPtr;

	if (a->freq > b->freq) return -1;
	else if (a->freq < b->freq) return 1;
	else return (uint32_t)a->c > (uint32_t)b->c;
}

static void FillDictionary(Symbol* symbols, char* dictionary)
{
	Buffer buffer = {0};

	// Read all symbols from standard input into 
	// the symbols table, counting frequency.
	while (1)
	{
		uint32_t c = ReadByte(&buffer);

		if (c == INVALID_BYTE)
			break;

		Symbol* sym = symbols + c;
		sym->c = c;
		++sym->freq;
	}

	// Sort the symbols table according to frequency.
	qsort(symbols, SYM_SIZE, sizeof(Symbol), CompareSymbols);

	for (int i = 1; i < DICT_SIZE; ++i)
	{
		if (symbols[i - 1].c == '\0')
			break;

		dictionary[i] = symbols[i - 1].c;
	}
}

// Returns true if the character 'c' is in the
// given dictionary.
static int InDictionary(char* dictionary, char c)
{
	for (int i = 1; i < DICT_SIZE; ++i)
	{
		if (dictionary[i] == c)
			return i;
	}

	return -1;
}

// Writes a bit in a buffered manner.
// Returns true if the buffered byte was written
// to disk, false otherwise.
static bool WriteBit(BitWriter* writer, uint32_t bit)
{
	// Make room for a new bit and OR that bit in.
	// (bit & 1) ensures only the least-significant bit
	// is set for the OR operation.
	writer->byte = (writer->byte << 1) | (bit & 1);

	// A byte has been filled, write it out to file.
	if (++writer->count == 8)
	{
		if (write(1, &writer->byte, 1) < 0)
		{
			PRINT("Failed to write the byte into the file. %s\n", strerror(errno));
			exit(-1);
		}

		writer->count = 0;
		return true;
	}

	return false;
}

// Writes 0 bits into the BitWriter until its byte
// is full and is output to file.
static void Flush(BitWriter* writer)
{
	while (!WriteBit(writer, 0));
}

// Returns the bit from 'value' at 'pos' position.
static uint32_t GetBit(uint32_t value, int pos)
{
    return (value >> pos) & 1;
}

int main()
{
	// Holds the most frequent symbols.
	char dictionary[DICT_SIZE] = {0};

	// Each character in ASCII indexes into here.
	// It tracks how many times we've seen it.
	Symbol symbols[SYM_SIZE] = {0};

	FillDictionary(symbols, dictionary);
	
	// Write dictionary to output file.
	if (write(1, dictionary + 1, DICT_SIZE - 1) < 0)
	{
		PRINT("Failed to write the dictionary into the file. %s\n", strerror(errno));
		return -1;
	}

	if (lseek(0, 0, SEEK_SET) < 0)
	{
		PRINT("Failed to seek the input file. %s\n", strerror(errno));
		return -1;
	}

	BitWriter writer = { 0, 0 };
	Buffer buffer = {0};

	// Encoding loop.
	while (1)
	{
		uint32_t c = ReadByte(&buffer);

		if (c == INVALID_BYTE)
			break;

		if (c == 0)
		{
			// Encode the 0 byte as '10'.
			WriteBit(&writer, 1);
			WriteBit(&writer, 0);
			continue;
		}

		int offset = InDictionary(dictionary, c);

		if (offset != -1)
		{
			// This character is in our dictionary, so we'll
			// encode it in a compressed manner.
			
			uint32_t runLength = 0;

			// Calculate the run-length. We already read the first
			// character, so we can read up to 3 more.
			for (int i = 0; i < 3; ++i)
			{
				uint32_t next = ReadByte(&buffer);

				if (next == INVALID_BYTE)
					break;

				if (next == c)
					++runLength;
				else
				{
					// Character didn't match, undo the read.
					--buffer.index;
					++buffer.size;
					break;
				}
			}

			// Output 8 bits: 11 followed by a 2 bit run-length
			// followed by a 4 bit offset in the dictionary.
			WriteBit(&writer, 1);
			WriteBit(&writer, 1);

			uint32_t run0 = GetBit(runLength, 0);
			uint32_t run1 = GetBit(runLength, 1);

			uint32_t off0 = GetBit(offset, 0);
			uint32_t off1 = GetBit(offset, 1);
			uint32_t off2 = GetBit(offset, 2);
			uint32_t off3 = GetBit(offset, 3);

			WriteBit(&writer, run0);
			WriteBit(&writer, run1);
			
			WriteBit(&writer, off0);
			WriteBit(&writer, off1);
			WriteBit(&writer, off2);
			WriteBit(&writer, off3);
		}
		else
		{
			// This character isn't in our dictionary.
			// Write a 0 bit, followed by the character itself.
			WriteBit(&writer, 0);

			for (int i = 0; i < 8; ++i)
			{
				uint32_t bit = GetBit(c, i);
				WriteBit(&writer, bit);
			}
		}
	}

	// Write EOF symbol.
	WriteBit(&writer, 1);
	WriteBit(&writer, 1);

	for (int i = 0; i < 6; ++i)
		WriteBit(&writer, 0);

	Flush(&writer);
}
