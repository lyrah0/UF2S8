#include "debugger.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "isa.h"

extern void print_state(
	const struct VirtualMachine *viM, uint16_t instruction);

static const char condition_str[8][3] = { "ZS", "ZC", "CS", "CC", "NS", "NC",
	"VS", "AL" };

static bool disassemble_loadstore(uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.loadstore.reg_dst;
	uint8_t reg_base = inst.loadstore.reg_base;
	int16_t imm_ls = sign_extend(inst.loadstore.offset, 7);

	if (inst.loadstore.opcode == 0xA) {
		printf("SB r%hhu, [a%hhu%c%hhd]", reg_dst, reg_base,
			imm_ls < 0 ? '-' : '+', imm_ls < 0 ? -imm_ls : imm_ls);
	} else if (inst.loadstore.opcode == 0xB) {
		printf("LB r%hhu, [a%hhu%c%hhd]", reg_dst, reg_base,
			imm_ls < 0 ? '-' : '+', imm_ls < 0 ? -imm_ls : imm_ls);
	} else {
		return false;
	}
	return true;
}

static bool disassemble_mov(uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.reg_dst;
	uint8_t reg_src = inst.reg_src;

	if (inst.reg2.opcode == 0x010) {
		printf("MOV r%hhu, ", reg_dst);
		switch (reg_src) {
		case 0:
			printf("flags");
			break;
		case 6:
			printf("spl");
			break;
		case 7:
			printf("sph");
			break;
		default:
			printf("???%hhu", reg_src);
		}
	} else if (inst.reg2.opcode == 0x090) {
		printf("MOV ");
		switch (reg_dst) {
		case 0:
			printf("flags, ");
			break;
		case 6:
			printf("spl, ");
			break;
		case 7:
			printf("sph, ");
			break;
		default:
			printf("???%hhu, ", reg_dst);
		}
		printf("r%hhu", reg_src);
	} else {
		return false;
	}
	return true;
}
static bool disassemble_branch(uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.branch.cond;
	uint8_t reg_base = inst.loadstore.reg_base;
	int16_t imm_ls = sign_extend(inst.loadstore.offset, 7);
	int16_t imm_brel = (int16_t)(sign_extend(inst.branch.offset, 9) << 1);

	if (inst.branch.opcode == 0xC) {
		printf("B %s, [a%hhu%c%hd]", condition_str[reg_dst], reg_base,
			imm_ls < 0 ? '-' : '+',
			imm_ls < 0 ? -(imm_ls << 1) : (imm_ls << 1));
	} else if (inst.branch.opcode == 0xD) {
		printf("B %s, %hd", condition_str[reg_dst], imm_brel);
	} else if (inst.branch.opcode == 0xE) {
		printf("BL %s, [a%hhu%c%hd]", condition_str[reg_dst], reg_base,
			imm_ls < 0 ? '-' : '+',
			imm_ls < 0 ? -(imm_ls << 1) : (imm_ls << 1));
	} else if (inst.branch.opcode == 0xF) {
		printf("BL %s, %hd", condition_str[reg_dst], imm_brel);
	} else {
		return false;
	}
	return true;
}

void disassemble(uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.reg_dst;
	uint8_t reg_src = inst.reg_src;
	uint8_t reg_mod = inst.reg_mod;
	uint8_t imm_li = sign_extend(inst.load_imm.imm, 8);
	int8_t imm_add = (int8_t)sign_extend(inst.addi.imm, 5);

	if (instruction == 0x0000) {
		printf("NOP");
	} else if (instruction == 0x2000) {
		printf("RET");
	} else if (instruction == 0x0400) {
		printf("RETI");
	} else if (inst.reg1.opcode == 0x1400) {
		printf("SWI r%hhu", reg_dst);
	} else if (inst.reg1.opcode == 0x0800) {
		printf("INCC r%hhu", reg_dst);
	} else if (inst.reg1.opcode == 0x0C00) {
		printf("DECB r%hhu", reg_dst);
	} else if (inst.reg1.opcode == 0x1800) {
		printf("POP r%hhu", reg_dst);
	} else if (inst.reg1.opcode == 0x1C00) {
		printf("PUSH r%hhu", reg_dst);
	} else if (disassemble_mov(instruction)) {
	} else if (inst.opcode == 0x10) {
		const char instructions[8][4] = { "???", "???", "CMP", "CMA",
			"???", "???", "???", "???" };
		printf("%s r%hhu, r%hhu",
			instructions[(instruction >> 7) & 0x7], reg_dst,
			reg_src);
	} else if (inst.branch.opcode == 0x1) {
		const char instructions[8][4] = { "SUB", "SBC", "ADD", "ADC",
			"AND", "OR", "NOR", "XOR" };
		printf("%s r%hhu, r%hhu, r%hhu",
			instructions[(instruction >> 4) & 0x7], reg_dst,
			reg_src, reg_mod);
	} else if (inst.branch.opcode == 0x2) {
		const char instructions[8][4] = { "SLL", "SRL", "SRA", "???",
			"SLL", "SRL", "SRA", "???" };
		printf("%s r%hhu, r%hhu, ",
			instructions[(instruction >> 4) & 0x7], reg_dst,
			reg_src);
		if ((instruction >> 4) & (0x7 > 3)) {
			printf("%hhu", reg_mod);
		} else {
			printf("r%hhu", reg_mod);
		}
	} else if (inst.load_imm.opcode == 0x09) {
		printf("LI r%hhu, 0x%02hhx", reg_dst, imm_li);
	} else if (inst.addi.opcode == 0x19) {
		printf("ADD r%hhu, r%hhu, %hhd", reg_dst, reg_src, imm_add);
	} else if (disassemble_loadstore(instruction)) {
	} else if (disassemble_branch(instruction)) {
	} else {
		printf("Unknown");
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
			viM->memory[(uint16_t)(address + i)]);
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
		uint16_t instruction = viM->memory[address + 1] << 8 |
			viM->memory[address];
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
			viM->memory[address] = value;
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
