#include "memory.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>

static void terminal_write(uint8_t value)
{
	//printf("put char!\n");
	putchar(value);
	(void)fflush(stdout);
}

uint16_t fetch_instruction(struct VirtualMachine *viM)
{
	uint16_t instruction = (viM->memory[(uint16_t)(viM->pc + 1)] << 8) |
		viM->memory[viM->pc];

	viM->pc += 2;

	return instruction;
}

void memory_dump(struct VirtualMachine *viM)
{
	if (viM->memory_dump) {
		for (int i = 0; i < MAX_MEMORY; i++) {
			printf("0x%04hx: %02hhx", (uint16_t)i,
				viM->memory[(uint16_t)i]);
			i++;
			while (i % 16 != 0) {
				printf(" %02hhx",
					viM->memory[(uint16_t)(i++)]);
			}
			printf("  ");
			while (i % 32 != 31) {
				printf(" %02hhx",
					viM->memory[(uint16_t)(i++)]);
			}
			printf(" %02hhx\n", viM->memory[(uint16_t)i]);
		}
	}
}

void memory_write(struct VirtualMachine *viM, uint16_t address, uint8_t value)
{
	if (address == HW_TERMINAL_OUT) {
		terminal_write(value);
	} else {
		viM->memory[address] = value;
	}
}