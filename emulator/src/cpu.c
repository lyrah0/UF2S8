#include "cpu.h"
#include "io.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "isa.h"
#include "memory.h"

void print_state(const struct VirtualMachine *viM, uint16_t instruction)
{
	int interrupt = (viM->csr[0] >> 7) & 1;
	int overflow = (viM->csr[0] >> 3) & 1;
	int negative = (viM->csr[0] >> 2) & 1;
	int zero = (viM->csr[0] >> 1) & 1;
	int carry = viM->csr[0] & 1;
	static unsigned int tick = 0;

	printf("Tick: %03x PC: 0x%04x\tinstruction: 0x%04x\tflags: I:%d V:%d N:%d Z:%d C:%d\n",
		tick++, (uint16_t)(viM->pc - 2), instruction, interrupt,
		overflow, negative, zero, carry);
	printf("r0:0x%02x r1:0x%02x r2:0x%02x r3:0x%02x r4:0x%02x r5:0x%02x r6:0x%02x r7:0x%02x sp:0x%02x%02x\n",
		viM->gpr[0], viM->gpr[1], viM->gpr[2], viM->gpr[3],
		viM->gpr[4], viM->gpr[5], viM->gpr[6], viM->gpr[7],
		viM->csr[7], viM->csr[6]);
	if (tick > 0xFF) { exit(0); }
}

static void flags_overflow_add(
	struct VirtualMachine *viM, uint16_t src, uint16_t mod, uint16_t res)
{
	if (((src ^ res) & (mod ^ res)) >> 15) {
		viM->csr[0] |= 1 << 3;
	} else {
		viM->csr[0] &= ~(1 << 3);
	}
}

static void flags_overflow_sub(
	struct VirtualMachine *viM, uint16_t src, uint16_t mod, uint16_t res)
{
	if (((src ^ mod) & (src ^ res)) >> 15) {
		viM->csr[0] |= 1 << 3;
	} else {
		viM->csr[0] &= ~(1 << 3);
	}
}

static bool get_cond(struct VirtualMachine *viM, uint16_t instruction)
{
	uint8_t reg_dst = (instruction >> 13) & 0x7;
	bool overflow = ((viM->csr[0] >> 3) & 1) != 0;
	bool negative = ((viM->csr[0] >> 2) & 1) != 0;
	bool zero = ((viM->csr[0] >> 1) & 1) != 0;
	bool carry = (viM->csr[0] & 1) != 0;

	switch (reg_dst) {
	case 0:
		return zero;
	case 1:
		return (!zero) != 0;
	case 2:
		return carry;
	case 3:
		return (!carry) != 0;
	case 4:
		return negative;
	case 5:
		return (!negative) != 0;
	case 6:
		return overflow;
	default:
		return true;
	}
}

static uint32_t execute_logic(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_src = inst.reg_src;
	uint8_t reg_mod = inst.reg_mod;

	if (inst.reg3.opcode == 0x41) {
		return viM->gpr[reg_src] & viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x51) {
		return viM->gpr[reg_src] | viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x61) {
		return ~(viM->gpr[reg_src] | viM->gpr[reg_mod]);
	}
	if (inst.reg3.opcode == 0x71) {
		return viM->gpr[reg_src] ^ viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x02) {
		return viM->gpr[reg_src] << viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x12) {
		return viM->gpr[reg_src] >> viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x22) {
		return (int8_t)viM->gpr[reg_src] >> viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x42) { return viM->gpr[reg_src] << reg_mod; }
	if (inst.reg3.opcode == 0x52) { return viM->gpr[reg_src] >> reg_mod; }
	if (inst.reg3.opcode == 0x62) {
		return (int8_t)viM->gpr[reg_src] >> reg_mod;
	}

	return 99999999;
}

static bool execute_flags(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.reg_dst;
	uint8_t reg_src = inst.reg_src;
	uint8_t reg_mod = inst.reg_mod;
	uint8_t reg_base = inst.loadstore.reg_base << 1;
	uint8_t imm_li = sign_extend(inst.load_imm.imm, 8);
	int8_t imm_add = (int8_t)sign_extend(inst.addi.imm, 5);
	int16_t imm_ls = sign_extend(inst.loadstore.offset, 7);
	uint32_t temp = execute_logic(viM, instruction);
	char sub_add = -1;
	bool write = true;

	if (temp != 99999999) {
	} else if ((instruction & 0x03FF) == 0x0080) {
		temp = viM->gpr[reg_src];
	} else if ((instruction & 0x03FF) == 0x0100) {
		temp = viM->csr[reg_src];
	} else if ((instruction & 0x1FFF) == 0x0800) {
		temp = viM->gpr[reg_dst] + (viM->csr[0] & 1);
	} else if ((instruction & 0x1FFF) == 0x0C00) {
		temp = viM->gpr[reg_dst] - (viM->csr[0] & 1);
	} else if ((instruction & 0x1FFF) == 0x1800) {
		uint16_t stackp = viM->csr[7] << 8 | viM->csr[6];
		temp = viM->memory[++stackp];
		viM->csr[7] = stackp >> 8;
		viM->csr[6] = stackp;
	} else if ((instruction & 0x03FF) == 0x0010) {
		temp = viM->gpr[reg_dst] - viM->gpr[reg_src];
		write = false;
	} else if ((instruction & 0x03FF) == 0x0090) {
		temp = viM->gpr[reg_dst] + viM->gpr[reg_src];
		write = false;
	} else if ((instruction & 0x03FF) == 0x0110) {
		temp = viM->gpr[reg_dst] & viM->gpr[reg_src];
		write = false;
	} else if ((instruction & 0x7F) == 0x01) {
		temp = viM->gpr[reg_src] - viM->gpr[reg_mod];
		sub_add = 0;
	} else if ((instruction & 0x7F) == 0x11) {
		temp = viM->gpr[reg_src] - viM->gpr[reg_mod] -
			(viM->csr[0] & 1);
		sub_add = 0;
	} else if ((instruction & 0x7F) == 0x21) {
		temp = viM->gpr[reg_src] + viM->gpr[reg_mod];
		sub_add = 1;
	} else if ((instruction & 0x7F) == 0x31) {
		temp = viM->gpr[reg_src] + viM->gpr[reg_mod] +
			(viM->csr[0] & 1);
		sub_add = 1;
	} else if ((instruction & 0x1F) == 0x09) {
		temp = imm_li;
	} else if ((instruction & 0x1F) == 0x19) {
		temp = viM->gpr[reg_src] + imm_add;
	} else if ((instruction & 0xF) == 0xB) {
		temp = viM->memory[(
			uint16_t)((viM->gpr[reg_base] |
					  viM->gpr[reg_base + 1] << 8) +
			imm_ls)];
	} else {
		return true;
	}

	if ((temp & 0xFFFF0000) != 0) {
		viM->csr[0] |= 0x01;
	} else {
		viM->csr[0] &= 0xFE;
	}
	if (temp == 0) {
		viM->csr[0] |= 0x02;
	} else {
		viM->csr[0] &= 0xFD;
	}
	if ((temp & 0x8000) != 0) {
		viM->csr[0] |= 0x04;
	} else {
		viM->csr[0] &= 0xFB;
	}

	switch (sub_add) {
	case 0:
		flags_overflow_sub(
			viM, viM->gpr[reg_src], viM->gpr[reg_mod], temp);
		break;
	case 1:
		flags_overflow_add(
			viM, viM->gpr[reg_src], viM->gpr[reg_mod], temp);
		break;
	default:
		viM->csr[0] &= ~(1 << 3);
	}
	if (write) { viM->gpr[reg_dst] = (uint8_t)temp; }

	return false;
}

static bool execute_branch(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_base = inst.loadstore.reg_base << 1;
	int16_t imm_ls = sign_extend(inst.loadstore.offset, 7);
	int16_t imm_brel = (int16_t)(sign_extend(inst.branch.offset, 9) << 1);
	uint16_t temp = 0;
	bool cond = get_cond(viM, instruction);

	if (instruction == 0x2000) {
		temp = viM->csr[7] << 8 | viM->csr[6];
		viM->pc = viM->memory[++temp];
		viM->pc |= viM->memory[++temp] << 8;
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if (instruction == 0x0400) {
		temp = viM->csr[7] << 8 | viM->csr[6];
		viM->csr[0] = viM->memory[++temp];
		viM->pc = viM->memory[++temp];
		viM->pc |= viM->memory[++temp] << 8;
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if ((instruction & 0xF) == 0xC) {
		if (cond) {
			viM->pc = (uint16_t)((viM->gpr[reg_base] |
						     viM->gpr[reg_base + 1]
							     << 8) +
				(imm_ls << 1));
		}
	} else if ((instruction & 0xF) == 0xD) {
		if (cond) { viM->pc += imm_brel; }
	} else if ((instruction & 0xF) == 0xE) {
		if (cond) {
			temp = viM->csr[7] << 8 | viM->csr[6];
			viM->memory[temp--] = viM->pc >> 8;
			viM->memory[temp--] = viM->pc;
			viM->pc = (uint16_t)((viM->gpr[reg_base] |
						     viM->gpr[reg_base + 1]
							     << 8) +
				(imm_ls << 1));
			viM->csr[7] = temp >> 8;
			viM->csr[6] = temp;
		}
	} else if ((instruction & 0xF) == 0xF) {
		if (cond) {
			temp = viM->csr[7] << 8 | viM->csr[6];
			viM->memory[temp--] = viM->pc >> 8;
			viM->memory[temp--] = viM->pc;
			viM->pc += imm_brel;
			viM->csr[7] = temp >> 8;
			viM->csr[6] = temp;
		}
	} else {
		return true;
	}

	return false;
}

bool decode_execute(struct VirtualMachine *viM, uint16_t instruction)
{
	uint8_t reg_dst = (instruction >> 13) & 0x7;
	uint8_t reg_src = (instruction >> 10) & 0x7;
	uint8_t reg_base = (instruction >> 10) & 0x6;
	int16_t imm_ls = sign_extend((instruction >> 4) & 0x7F, 7);
	uint16_t temp = 0;

	if (instruction == 0) { return false; }
	if (!execute_branch(viM, instruction)) { return false; }
	if (!execute_flags(viM, instruction)) { return false; }

	if ((instruction & 0x1FFF) == 0x1400) {
		interrupt_pushtostack(viM);
		uint16_t vec_addr = 0xFF00 + ((viM->gpr[reg_dst] & 0x7F) << 1);
		viM->pc = viM->memory[vec_addr] |
			(viM->memory[vec_addr + 1] << 8);
		if ((viM->gpr[reg_dst] & 0x7F) == 0) { viM->running = false; }
		if ((viM->gpr[reg_dst] & 0x7F) == 1) {
			viM->debug_mode = true;
		}
	} else if ((instruction & 0x1FFF) == 0x1C00) {
		temp = viM->csr[7] << 8 | viM->csr[6];
		viM->memory[temp--] = viM->gpr[reg_dst];
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if ((instruction & 0x03FF) == 0x0180) {
		viM->csr[reg_dst] = viM->gpr[reg_src];
	} else if ((instruction & 0xF) == 0xA) {
		temp = (viM->gpr[reg_base] | viM->gpr[reg_base + 1] << 8) +
			imm_ls;
		memory_write(viM, temp, viM->gpr[reg_dst]);
	} else {
		printf("ERROR: illegal instruction: 0x%04x\n", instruction);
		return true;
	}

	return false;
}
