#include "cpu.h"
#include <cstdio>

static Instruction DecodeInstruction(uint32_t value)
{
	Instruction inst = {};
	inst.id = (value >> 17) & 0x00007;
	inst.dst = (value >> 14) & 0x00007;
	inst.src = (value >> 11) & 0x00007;
	inst.target = (value >> 8) & 0x00007;
	inst.imm = (value & 0x000FF);

	return inst;
}

void CPU::Reset()
{
	memset(registers, 0, sizeof(registers));
	PC = 0;
	TC = 0;
	halted = false;
}

void CPU::DoTick(uint16_t count)
{
	if (!halted)
	{
		++TC;

		if (count >= waitingForTick)
		{
			Instruction instruction = DecodeInstruction(instructions->Fetch(PC));
			RunInstruction(instruction);

			int wait = instructions->GetTiming(instruction);
			waitingForTick += wait;
		}
	}
}

void CPU::LoadWord(Instruction i)
{
	uint8_t byte = source->GetByte(registers[i.target]);
	waitingForTick += source->GetWait();
	registers[i.dst] = byte;
	++PC;
}

void CPU::StoreWord(Instruction i)
{
	uint8_t byte = registers[i.src];
	source->SetByte(registers[i.target], byte);
	waitingForTick += source->GetWait();
	++PC;
}

void CPU::Add(Instruction i)
{
	int8_t target = (int8_t)registers[i.target];
	int8_t result = (int8_t)registers[i.src] + target;
	registers[i.dst] = result;
	++PC;
}

void CPU::AddImmediate(Instruction i)
{
	int8_t result = (int8_t)registers[i.src] + i.imm;
	registers[i.dst] = result;
	++PC;
}

void CPU::Multiply(Instruction i)
{
	int8_t src = registers[i.src];

	uint32_t lower = src & 0x0F;
	uint32_t upper = (src >> 4) & 0x0F;

	uint32_t result = lower * upper;

	registers[i.dst] = (int8_t)result;
	++PC;
}

void CPU::Invert(Instruction i)
{
	uint8_t result = ~registers[i.src];
	registers[i.dst] = result;
	++PC;
}

void CPU::BranchIfEqual(Instruction i)
{
	if (registers[i.src] == registers[i.target])
	{
		PC = i.imm;
		++waitingForTick;
	}
	else ++PC;
}

void CPU::Halt()
{
	++PC;
	halted = true;
}

void CPU::RunInstruction(Instruction inst)
{
	switch (inst.id)
	{
		case LOAD_WORD:
			LoadWord(inst);
			break;

		case STORE_WORD:
			StoreWord(inst);
			break;

		case ADD:
			Add(inst);
			break;

		case ADDI:
			AddImmediate(inst);
			break;

		case MUL:
			Multiply(inst);
			break;

		case INV:
			Invert(inst);
			break;

		case BEQ:
			BranchIfEqual(inst);
			break;

		case HALT:
			Halt();
			break;

	}
}

void CPU::SetReg(char* reg, uint8_t value)
{
	if (reg[0] == 'P')
	{
		PC = value;
		waitingForTick = 0;
	}
	else
	{
		int diff = reg[1] - 'A';
		registers[diff] = value;
	}
}

void CPU::Dump()
{
	printf("PC: 0x%02X\n", PC);

	for (int i = 0; i < 8; ++i)
		printf("R%c: 0x%02X\n", 'A' + i, registers[i]);
	
	printf("TC: %d\n", TC);
	printf("\n");
}

void CPU::RunCommand(std::vector<char*> args)
{
	if (StringEquals(args[1], "reset"))
		Reset();
	else if (StringEquals(args[1], "set") && StringEquals(args[2], "reg"))
	{
		uint8_t byte = (uint8_t)HexToInt(args[4]);
		SetReg(args[3], byte);
	}
	else if (StringEquals(args[1], "dump"))
		Dump();
}
