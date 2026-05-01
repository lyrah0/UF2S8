#include "cpu.h"
#include "io.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>

#include "isa.h"
#include "memory.h"

void print_state(const struct VirtualMachine *viM, uint16_t instruction)
{
	int interrupt = (viM->csr[0] >> 7) & 1;
	int overflow = (viM->csr[0] >> 3) & 1;
	int negative = (viM->csr[0] >> 2) & 1;
	int zero = (viM->csr[0] >> 1) & 1;
	int carry = viM->csr[0] & 1;

	printf("PC: 0x%04x\tinstruction: 0x%04x\tflags: I:%d V:%d N:%d Z:%d C:%d\n",
		(uint16_t)(viM->pc - 2), instruction, interrupt, overflow,
		negative, zero, carry);
	printf("r0:0x%02x r1:0x%02x r2:0x%02x r3:0x%02x r4:0x%02x r5:0x%02x r6:0x%02x r7:0x%02x sp:0x%02x%02x\n",
		viM->gpr[0], viM->gpr[1], viM->gpr[2], viM->gpr[3],
		viM->gpr[4], viM->gpr[5], viM->gpr[6], viM->gpr[7],
		viM->csr[7], viM->csr[6]);
}

static void flags_overflow_add(
	struct VirtualMachine *viM, uint16_t src, uint16_t mod, uint16_t res)
{
	if (((src ^ res) & (mod ^ res)) & 0x80) {
		viM->csr[0] |= 1 << 3;
	} else {
		viM->csr[0] &= ~(1 << 3);
	}
}

static void flags_overflow_sub(
	struct VirtualMachine *viM, uint16_t src, uint16_t mod, uint16_t res)
{
	if (((src ^ mod) & (src ^ res)) & 0x80) {
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

	if (inst.reg3.opcode == 0x41) { // AND
		return viM->gpr[reg_src] & viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x51) { // OR
		return viM->gpr[reg_src] | viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x61) { // NOR
		return ~(viM->gpr[reg_src] | viM->gpr[reg_mod]);
	}
	if (inst.reg3.opcode == 0x71) { // XOR
		return viM->gpr[reg_src] ^ viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x02) { // SLL
		return viM->gpr[reg_src] << viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x12) { // SRL
		return viM->gpr[reg_src] >> viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x22) { // SRA
		return (int8_t)viM->gpr[reg_src] >> viM->gpr[reg_mod];
	}
	if (inst.reg3.opcode == 0x42) { // SLL immediate
		return viM->gpr[reg_src] << reg_mod;
	}
	if (inst.reg3.opcode == 0x52) { // SRL immediate
		return viM->gpr[reg_src] >> reg_mod;
	}
	if (inst.reg3.opcode == 0x62) { // SRA immediate
		return (int8_t)viM->gpr[reg_src] >> reg_mod;
	}

	return 9999;
}

static bool execute_flags(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.reg_dst;
	uint8_t reg_src = inst.reg_src;
	uint8_t reg_mod = inst.reg_mod;
	uint8_t reg_base = inst.loadstore.reg_base << 1;
	uint16_t temp = execute_logic(viM, instruction);
	char sub_add = -1;
	bool write = true;

	if (temp != 9999) {
	} else if (inst.reg2.opcode == 0x0010) { // MOV reg, csr
		temp = viM->csr[reg_src];
	} else if (inst.reg1.opcode == 0x0800) { // INCC
		temp = viM->gpr[reg_dst] + (viM->csr[0] & 1);
	} else if (inst.reg1.opcode == 0x0C00) { // DECB
		temp = (uint16_t)viM->gpr[reg_dst] + 0xFF +
			(viM->csr[0] & 0x01);
	} else if (inst.reg1.opcode == 0x1800) { // POP
		uint16_t stackp = viM->csr[7] << 8 | viM->csr[6];
		temp = memory_read(viM, ++stackp);
		viM->csr[7] = stackp >> 8;
		viM->csr[6] = stackp;
	} else if ((inst.raw & 0x3FFF) == 0x0380) { // POP a
		uint16_t stackp = viM->csr[7] << 8 | viM->csr[6];
		temp = memory_read(viM, ++stackp);
		viM->gpr[reg_dst & 0x6] = memory_read(viM, ++stackp);
		reg_dst = (reg_dst & 0x6) + 1;
		viM->csr[7] = stackp >> 8;
		viM->csr[6] = stackp;
	} else if (inst.reg2.opcode == 0x0110) { // CMP
		temp = (uint16_t)viM->gpr[reg_dst] +
			(uint16_t)(~viM->gpr[reg_src] & 0xFF) + 1;
		write = false;
	} else if (inst.reg2.opcode == 0x0190) { // CMA
		temp = viM->gpr[reg_dst] & viM->gpr[reg_src];
		write = false;
	} else if (inst.reg3.opcode == 0x01) { // SUB
		temp = (uint16_t)viM->gpr[reg_src] +
			(uint16_t)(~viM->gpr[reg_mod] & 0xFF) + 1;
		sub_add = 0;
	} else if (inst.reg3.opcode == 0x11) { // SBB
		temp = (uint16_t)viM->gpr[reg_src] +
			(uint16_t)(~viM->gpr[reg_mod] & 0xFF) +
			(viM->csr[0] & 0x01);
		sub_add = 0;
	} else if (inst.reg3.opcode == 0x21) { // ADD
		temp = viM->gpr[reg_src] + viM->gpr[reg_mod];
		sub_add = 1;
	} else if (inst.reg3.opcode == 0x31) { // ADC
		temp = (uint16_t)viM->gpr[reg_src] +
			(uint16_t)viM->gpr[reg_mod] + (viM->csr[0] & 0x01);
		sub_add = 1;
	} else if (inst.load_imm.opcode == 0x0A) { // LI
		temp = sign_extend(inst.load_imm.imm, 8);
	} else if (inst.addi.opcode == 0xB) { // ADDI
		temp = (uint16_t)viM->gpr[reg_src] +
			(uint16_t)(uint8_t)sign_extend(inst.addi.imm, 6);
	} else if (inst.loadstore.opcode == 0xD) { // LB
		temp = memory_read(viM,
			(uint16_t)((viM->gpr[reg_base] |
					   viM->gpr[reg_base + 1] << 8) +
				sign_extend(inst.loadstore.offset, 7)));
	} else {
		return false;
	}

	if ((temp & 0xFF00) != 0) {
		viM->csr[0] |= 0x01;
	} else {
		viM->csr[0] &= 0xFE;
	}
	if ((uint8_t)temp == 0) {
		viM->csr[0] |= 0x02;
	} else {
		viM->csr[0] &= 0xFD;
	}
	if ((temp & 0x0080) != 0) {
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

	return true;
}

static bool execute_branch(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_base = inst.loadstore.reg_base << 1;
	uint16_t temp = 0;

	if (instruction == 0x2000) { // RET
		temp = viM->csr[7] << 8 | viM->csr[6];
		viM->pc = memory_read(viM, ++temp);
		viM->pc |= memory_read(viM, ++temp) << 8;
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if (instruction == 0x4000) { // WFI
		viM->wait_for_interrupt = true;
	} else if (instruction == 0x0400) { // RETI
		temp = viM->csr[7] << 8 | viM->csr[6];
		viM->csr[0] = memory_read(viM, ++temp);
		viM->pc = memory_read(viM, ++temp);
		viM->pc |= memory_read(viM, ++temp) << 8;
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if ((inst.raw & 0x07FF) == 0x0070) { // B reg
		if (get_cond(viM, instruction)) {
			viM->pc = (uint16_t)(viM->gpr[reg_base] |
				viM->gpr[reg_base + 1] << 8);
		}
	} else if ((inst.raw & 0x07FF) == 0x0470) { // BL reg
		if (get_cond(viM, instruction)) {
			temp = viM->csr[7] << 8 | viM->csr[6];
			memory_write(viM, temp--, viM->pc >> 8);
			memory_write(viM, temp--, viM->pc);
			viM->pc = (uint16_t)(viM->gpr[reg_base] |
				viM->gpr[reg_base + 1] << 8);
			viM->csr[7] = temp >> 8;
			viM->csr[6] = temp;
		}
	} else if (inst.branch.opcode == 0xE) { // B
		if (get_cond(viM, instruction)) {
			viM->pc += (int16_t)(sign_extend(inst.branch.offset, 9)
				<< 1);
		}
	} else if (inst.branch.opcode == 0xF) { // BL
		if (get_cond(viM, instruction)) {
			temp = viM->csr[7] << 8 | viM->csr[6];
			memory_write(viM, temp--, viM->pc >> 8);
			memory_write(viM, temp--, viM->pc);
			viM->pc += (int16_t)(sign_extend(inst.branch.offset, 9)
				<< 1);
			viM->csr[7] = temp >> 8;
			viM->csr[6] = temp;
		}
	} else {
		return false;
	}

	return true;
}

bool decode_execute(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.reg3.reg_dst;
	uint8_t reg_src = inst.reg3.reg_src;
	uint8_t reg_base = inst.loadstore.reg_base << 1;
	uint16_t temp = 0;

	if (instruction == 0) { return false; }
	if (execute_branch(viM, instruction)) { return false; }
	if (execute_flags(viM, instruction)) { return false; }

	if (inst.reg1.opcode == 0x1400) { // SWI
		interrupt_pushtostack(viM);
		uint16_t vec_addr = 0xFF00 + ((viM->gpr[reg_dst] & 0x7F) << 1);
		viM->pc = memory_read(viM, vec_addr) |
			(memory_read(viM, vec_addr + 1) << 8);
		if ((viM->gpr[reg_dst] & 0x7F) == 0) { viM->running = false; }
		if ((viM->gpr[reg_dst] & 0x7F) == 1) {
			viM->debug_mode = true;
		}
	} else if (inst.reg1.opcode == 0x1C00) { // Push
		temp = viM->csr[7] << 8 | viM->csr[6];
		memory_write(viM, temp--, viM->gpr[reg_dst]);
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if ((inst.raw & 0x3FFF) == 0x2380) { // Push a
		temp = viM->csr[7] << 8 | viM->csr[6];
		memory_write(viM, temp--, viM->gpr[(reg_dst & 0x6) + 1]);
		memory_write(viM, temp--, viM->gpr[reg_dst & 0x6]);
		viM->csr[7] = temp >> 8;
		viM->csr[6] = temp;
	} else if (inst.reg2.opcode == 0x0090) { // MOV csr, reg
		viM->csr[reg_dst] = viM->gpr[reg_src];
	} else if (inst.loadstore.opcode == 0xC) { // SB
		temp = (viM->gpr[reg_base] | viM->gpr[reg_base + 1] << 8) +
			sign_extend(inst.loadstore.offset, 7);
		memory_write(viM, temp, viM->gpr[reg_dst]);
	} else {
		interrupt_pushtostack(viM);
		viM->pc = 0xF04;
		// end program anyways
		printf("ERROR: illegal instruction: 0x%04x\n", instruction);
		return true;
	}

	return false;
}
