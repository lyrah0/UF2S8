#include "debugger.h"
#include "memory.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isa.h"

extern void print_state(
	const struct VirtualMachine *viM, uint16_t instruction);

static const char condition_str[16][3] = { "EQ", "NE", "CS", "CC", "MI", "PL",
	"VS", "VC", "HI", "LS", "GE", "LT", "GT", "LE", "AS", "AC" };

static const char *get_csr_name(uint8_t csr)
{
	switch (csr) {
	case 0:
		return "flags";
	case 1:
		return "vbr";
	case 14:
		return "spl";
	case 15:
		return "sph";
	default:
		return "???";
	}
}

static bool disassemble_loadstore(uint16_t instruction)
{
	uint8_t op = instruction & 0x0F;
	uint8_t reg = (uint8_t)((instruction >> 4) & 0x0F);
	uint8_t base = (uint8_t)((instruction >> 9) & 0x07);
	int16_t off = sign_extend((int16_t)unpack_imm5(instruction), 5);

	if (op == 0xA) {
		printf("SB r%hhu, [a%hhu%c%hhd]", reg, base,
			off < 0 ? '-' : '+', off < 0 ? -off : off);
		return true;
	}
	if (op == 0xB) {
		printf("LB r%hhu, [a%hhu%c%hhd]", reg, base,
			off < 0 ? '-' : '+', off < 0 ? -off : off);
		return true;
	}
	return false;
}

static bool disassemble_mov(uint16_t instruction)
{
	if ((instruction & 0x0F) != 0x0) { return false; }
	uint8_t hi = (uint8_t)((instruction >> 12) & 0x0F);
	uint8_t src = (uint8_t)((instruction >> 8) & 0x0F);
	uint8_t dst = (uint8_t)((instruction >> 4) & 0x0F);

	if (hi == 0x3) {
		printf("MOV r%hhu, r%hhu", dst, src);
		return true;
	}
	if (hi == 0xA) {
		printf("MOV r%hhu, %s", dst, get_csr_name(src));
		return true;
	}
	if (hi == 0xB) {
		printf("MOV %s, r%hhu", get_csr_name(dst), src);
		return true;
	}
	return false;
}

static bool disassemble_branch(uint16_t instruction)
{
	uint8_t op = instruction & 0x0F;
	if (op == 0xC) {
		uint8_t cond = (uint8_t)((instruction >> 4) & 0x0F);
		int16_t off =
			(int16_t)(sign_extend(
					  (int16_t)unpack_imm8(instruction), 8)
				<< 1);
		printf("B %s, %hd", condition_str[cond], off);
		return true;
	}
	if (op == 0xD) {
		uint8_t cond = (uint8_t)((instruction >> 4) & 0x0F);
		int16_t off =
			(int16_t)(sign_extend(
					  (int16_t)unpack_imm8(instruction), 8)
				<< 1);
		printf("BL %s, %hd", condition_str[cond], off);
		return true;
	}
	if (op == 0xE) {
		int16_t off = (int16_t)(sign_extend((int16_t)unpack_imm12(
							    instruction),
						12)
			<< 1);
		printf("B %hd", off);
		return true;
	}
	if (op == 0xF) {
		int16_t off = (int16_t)(sign_extend((int16_t)unpack_imm12(
							    instruction),
						12)
			<< 1);
		printf("BL %hd", off);
		return true;
	}
	if ((instruction & 0xFF1F) == 0x1D00) {
		uint8_t b = (uint8_t)((instruction >> 5) & 0x07);
		printf("B a%hhu", b);
		return true;
	}
	if ((instruction & 0xFF1F) == 0x1D10) {
		uint8_t b = (uint8_t)((instruction >> 5) & 0x07);
		printf("BL a%hhu", b);
		return true;
	}
	if ((instruction & 0xF10F) == 0x2000) {
		uint8_t b = (uint8_t)((instruction >> 9) & 0x07);
		uint8_t cond = (uint8_t)((instruction >> 4) & 0x0F);
		printf("B %s, a%hhu", condition_str[cond], b);
		return true;
	}
	if ((instruction & 0xF10F) == 0x2100) {
		uint8_t b = (uint8_t)((instruction >> 9) & 0x07);
		uint8_t cond = (uint8_t)((instruction >> 4) & 0x0F);
		printf("BL %s, a%hhu", condition_str[cond], b);
		return true;
	}
	return false;
}

void disassemble(uint16_t instruction)
{
	uint8_t op = instruction & 0x0F;
	uint8_t dst = (uint8_t)((instruction >> 4) & 0x0F);
	uint8_t mod = (uint8_t)((instruction >> 8) & 0x0F);
	uint8_t hi = (uint8_t)((instruction >> 12) & 0x0F);

	if (instruction == 0x0000) {
		printf("NOP");
	} else if (instruction == 0x0010) {
		printf("RET");
	} else if (instruction == 0x0020) {
		printf("WFI");
	} else if (instruction == 0x0030) {
		printf("RETI");
	} else if ((instruction & 0xFF0F) == 0x1100) {
		printf("SWI r%hhu", dst);
	} else if ((instruction & 0xFF0F) == 0x1200) {
		printf("PUSH r%hhu", dst);
	} else if ((instruction & 0xFF0F) == 0x1300) {
		printf("POP r%hhu", dst);
	} else if ((instruction & 0xFF1F) == 0x1E00) {
		printf("BST flags, %hhu",
			(uint8_t)((instruction >> 5) & 0x07));
	} else if ((instruction & 0xFF1F) == 0x1E10) {
		printf("BIC flags, %hhu",
			(uint8_t)((instruction >> 5) & 0x07));
	} else if ((instruction & 0xFF1F) == 0x1F00) {
		printf("BTS flags, %hhu",
			(uint8_t)((instruction >> 5) & 0x07));
	} else if (disassemble_branch(instruction)) {
	} else if (disassemble_mov(instruction)) {
	} else if (op == 0x0) {
		if ((instruction & 0xF10F) == 0x4000) {
			printf("BST r%hhu, %hhu", dst,
				(uint8_t)((instruction >> 9) & 0x7));
		} else if ((instruction & 0xF10F) == 0x4100) {
			printf("BIC r%hhu, %hhu", dst,
				(uint8_t)((instruction >> 9) & 0x7));
		} else if (hi == 0x6) {
			printf("NOT r%hhu, r%hhu", dst, mod);
		} else if (hi == 0x7) {
			printf("NEG r%hhu, r%hhu", dst, mod);
		} else if (hi == 0x8) {
			printf("INC r%hhu, r%hhu", dst, mod);
		} else if (hi == 0x9) {
			printf("DEC r%hhu, r%hhu", dst, mod);
		} else if (hi == 0xC) {
			printf("INCC r%hhu, r%hhu", dst, mod);
		} else if (hi == 0xD) {
			printf("DECB r%hhu, r%hhu", dst, mod);
		} else if (hi == 0xE) {
			printf("CMP r%hhu, r%hhu", mod, dst);
		} else if (hi == 0xF) {
			printf("CMA r%hhu, r%hhu", mod, dst);
		} else {
			printf("??? (op0 0x%04hx)", instruction);
		}
	} else if (op == 0x1) {
		const char *alu_names[] = { "SUB", "SBB", "ADD", "ADC", "AND",
			"ANDN", "OR", "ORN", "NOR", "XOR", "SLL", "SRL",
			"SRA" };
		if (hi <= 0xC) {
			printf("%s r%hhu, r%hhu", alu_names[hi], dst, mod);
		} else if (hi == 0xD) {
			printf("SLL r%hhu, %hhu", dst,
				(uint8_t)((instruction >> 9) & 7));
		} else if (hi == 0xE) {
			printf("SRL r%hhu, %hhu", dst,
				(uint8_t)((instruction >> 9) & 7));
		} else if (hi == 0xF) {
			if (instruction & 0x0100) {
				printf("BTS r%hhu, %hhu", dst,
					(uint8_t)((instruction >> 9) & 7));
			} else {
				printf("SRA r%hhu, %hhu", dst,
					(uint8_t)((instruction >> 9) & 7));
			}
		}
	} else if (op == 0x2) {
		if (hi == 0x0) {
			printf("BST r%hhu, r%hhu", dst, mod);
		} else if (hi == 0x1) {
			printf("BIC r%hhu, r%hhu", dst, mod);
		} else if (hi == 0x2) {
			printf("BTS r%hhu, r%hhu", dst, mod);
		} else {
			printf("??? (op2 0x%04hx)", instruction);
		}
	} else if (op == 0x8) {
		printf("ADD r%hhu, %hhd", dst,
			(int8_t)unpack_imm8(instruction));
	} else if (op == 0x9) {
		printf("LI r%hhu, 0x%02hhx", dst, unpack_imm8(instruction));
	} else if (disassemble_loadstore(instruction)) {
	} else {
		printf("Unknown 0x%04hx", instruction);
	}
	printf("\n");
}

static void debug_prompt_b(struct VirtualMachine *viM, char *input)
{
	uint16_t address = 0;
	int value = 0;
	char *ptr = input + 1;
	char *endptr = nullptr;
	if (strncmp(ptr, " d ", 3) == 0) {
		value = (int)strtol(ptr + 3, nullptr, 10);
		if (value >= 0 && value < viM->bp_count) {
			for (int i = value; i < viM->bp_count - 1; i++) {
				viM->breakpoint[i] = viM->breakpoint[i + 1];
			}
			viM->bp_count--;
			printf("Breakpoint [%d] deleted.\n", value);
		} else {
			printf("Error: Invalid index %d\n", value);
		}
	} else if (input[2] == 'c') {
		viM->bp_count = 0;
		printf("All breakpoints cleared.\n");
	} else {
		address = (uint16_t)strtoul(ptr, &endptr, 16);
		if (ptr != endptr) {
			if (viM->bp_count >= MAX_BREAKPOINTS) {
				printf("Error: Maximum breakpoints reached! (%d)\n",
					viM->bp_count);
				return;
			}
			viM->breakpoint[viM->bp_count] = address;
			printf("Breakpoint [%d] set at 0x%04x\n",
				viM->bp_count, address);
			viM->bp_count++;
		} else {
			printf("Active breakpoints:\n");
			if (viM->bp_count == 0) {
				printf("(none)\n");
				return;
			}
			for (int i = 0; i < viM->bp_count; i++) {
				printf("[%d] 0x%04x\n", i, viM->breakpoint[i]);
			}
		}
	}
}

static void debug_prompt_m(struct VirtualMachine *viM, char *input)
{
	char *endptr = nullptr;
	uint16_t address = (uint16_t)strtoul(input + 1, &endptr, 16);
	int length = (int)strtol(endptr, nullptr, 10);
	for (int i = 0; i < length; i++) {
		printf("0x%04x: 0x%02x\n", (uint16_t)(address + i),
			memory_read(viM, (uint16_t)(address + i)));
	}
}

static void debug_prompt_s(char *input, int *length)
{
	*length = (int)strtol(input + 1, nullptr, 10);
	if (*length < 2) {
		*length = 0;
	} else {
		(*length)--;
	}
}

static void debug_prompt_l(struct VirtualMachine *viM, const char *input)
{
	int length = 8;
	uint16_t address = 0;
	char *endptr = nullptr;
	address = (uint16_t)strtoul(input + 1, &endptr, 16);
	if (input + 1 != endptr) {
		length = (int)strtol(endptr, nullptr, 10);
	} else {
		address = viM->pc - 2;
	}
	for (int i = 0; i < length; i++) {
		uint16_t instruction =
			(uint16_t)(memory_read(viM, address + 1) << 8 |
				memory_read(viM, address));
		printf("0x%04hx:  0x%04hx  ", address, instruction);
		disassemble(instruction);
		address += 2;
	}
}

void debug_prompt(struct VirtualMachine *viM, uint16_t instruction)
{
	char input[256];
	uint16_t address = 0;
	uint8_t value = 0;
	static int length = 0;
	if (length > 0) {
		length--;
		return;
	}
	printf("\n--- Debugger Paused ---\n");
	disassemble(instruction);
	printf("\n");
	while (true) {
		printf("(db) > ");
		if (!fgets(input, 256, stdin)) { break; }
		if (*input == 's') {
			debug_prompt_s(input, &length);
			break;
		}
		if (*input == 'm') {
			debug_prompt_m(viM, input);
		} else if (*input == 'b') {
			debug_prompt_b(viM, input);
		} else if (*input == 'w') {
			char *endptr = nullptr;
			address = (uint16_t)strtoul(input + 1, &endptr, 16);
			value = (uint8_t)strtoul(endptr, nullptr, 16);
			memory_write(viM, address, value);
		} else if (*input == 'l') {
			debug_prompt_l(viM, input);
		} else if (*input == 'p') {
			print_state(viM, instruction);
		} else if (*input == 'c') {
			viM->debug_mode = false;
			return;
		} else if (*input == 'q') {
			viM->running = false;
			break;
		}
	}
}
