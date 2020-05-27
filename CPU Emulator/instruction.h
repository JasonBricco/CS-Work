#pragma once

enum
{
	ADD,
	ADDI,
	MUL,
	INV,
	BEQ,
	LOAD_WORD,
	STORE_WORD,
	HALT,
	MAX_INSTRUCTIONS
};

struct Instruction
{
	int id;
	int dst;
	int src;
	int target;
	int imm;
};
