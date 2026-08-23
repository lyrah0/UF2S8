#include "cpu.h"
#include "io.h"
#include "vm.h"
#include "isa.h"
#include "memory.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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

static inline bool check_condition(
	const struct VirtualMachine *viM, uint8_t cond)
{
	uint8_t flags = viM->csr[0];
	switch (cond) {
	case 0: // ZS / EQ
		return (flags & 0x02) != 0;
	case 1: // ZC / NE
		return (flags & 0x02) == 0;
	case 2: // CS / HS
		return (flags & 0x01) != 0;
	case 3: // CC / LO
		return (flags & 0x01) == 0;
	case 4: // NS / MI
		return (flags & 0x04) != 0;
	case 5: // NC / PL
		return (flags & 0x04) == 0;
	case 6: // VS
		return (flags & 0x08) != 0;
	default: // AL
		return true;
	}
}

static inline void set_flags_add(
	struct VirtualMachine *viM, uint8_t a, uint8_t b, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0xF0;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	if (((a ^ res) & (b ^ res)) & 0x80) { flags |= 0x08; }
	viM->csr[0] = flags;
}

static inline void set_flags_sub(
	struct VirtualMachine *viM, uint8_t a, uint8_t b, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0xF0;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	if (((a ^ b) & (a ^ res)) & 0x80) { flags |= 0x08; }
	viM->csr[0] = flags;
}

static inline void set_flags_logic(struct VirtualMachine *viM, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0xF0;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	viM->csr[0] = flags;
}

static inline void push_word(struct VirtualMachine *viM, uint16_t val)
{
	uint16_t sp = (uint16_t)((viM->csr[7] << 8) | viM->csr[6]);
	memory_write(viM, sp--, (uint8_t)(val >> 8));
	memory_write(viM, sp--, (uint8_t)(val & 0xFF));
	viM->csr[7] = (uint8_t)(sp >> 8);
	viM->csr[6] = (uint8_t)sp;
}

static inline uint16_t pop_word(struct VirtualMachine *viM)
{
	uint16_t sp = (uint16_t)((viM->csr[7] << 8) | viM->csr[6]);
	uint8_t low = memory_read(viM, ++sp);
	uint8_t high = memory_read(viM, ++sp);
	viM->csr[7] = (uint8_t)(sp >> 8);
	viM->csr[6] = (uint8_t)sp;
	return (uint16_t)(low | (high << 8));
}

static bool execute_op0(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_src = inst.src;
	uint8_t reg_dst = inst.dst;
	uint8_t reg_mod = inst.mod;
	uint8_t reg_base = (uint8_t)(inst.sb.base << 1);

	if (instruction == 0x0000) { // NOP
		return false;
	}
	if (instruction == 0x2000) { // RET
		viM->pc = pop_word(viM);
		return false;
	}
	if (instruction == 0x4000) { // WFI
		viM->wait_for_interrupt = true;
		return false;
	}
	if (instruction == 0x0400) { // RETI
		uint16_t sp = (uint16_t)((viM->csr[7] << 8) | viM->csr[6]);
		viM->csr[0] = memory_read(viM, ++sp);
		uint8_t low = memory_read(viM, ++sp);
		uint8_t high = memory_read(viM, ++sp);
		viM->pc = (uint16_t)(low | (high << 8));
		viM->csr[7] = (uint8_t)(sp >> 8);
		viM->csr[6] = (uint8_t)sp;
		return false;
	}

	if ((inst.raw & 0x1CFF) == 0x0070) { // B reg
		if (check_condition(viM, inst.dst)) {
			viM->pc = (uint16_t)(viM->gpr[reg_base] |
				(viM->gpr[reg_base + 1] << 8));
		}
		return false;
	}
	if ((inst.raw & 0x1CFF) == 0x00F0) { // BL reg
		if (check_condition(viM, inst.dst)) {
			push_word(viM, viM->pc);
			viM->pc = (uint16_t)(viM->gpr[reg_base] |
				(viM->gpr[reg_base + 1] << 8));
		}
		return false;
	}

	if (inst.ss.op == 0x080 && inst.ss.op1 == 0x0) { // SWI
		interrupt_pushtostack(viM);
		uint8_t swi_id = viM->gpr[reg_src] & 0x7F;
		uint16_t vec_addr = (uint16_t)(0xFF00 + (swi_id << 1));
		viM->pc = (uint16_t)(memory_read(viM, vec_addr) |
			(memory_read(viM, (uint16_t)(vec_addr + 1)) << 8));
		if (swi_id == 0) { viM->running = false; }
		if (swi_id == 1) { viM->debug_mode = true; }
		return false;
	}
	if (inst.ss.op == 0x080 && inst.ss.op1 == 0x1) { // PUSH
		uint16_t sp = (uint16_t)((viM->csr[7] << 8) | viM->csr[6]);
		memory_write(viM, sp--, viM->gpr[reg_src]);
		viM->csr[7] = (uint8_t)(sp >> 8);
		viM->csr[6] = (uint8_t)sp;
		return false;
	}
	if (inst.sd.op == 0x0100) { // POP
		uint16_t sp = (uint16_t)((viM->csr[7] << 8) | viM->csr[6]);
		uint8_t val = memory_read(viM, ++sp);
		viM->csr[7] = (uint8_t)(sp >> 8);
		viM->csr[6] = (uint8_t)sp;
		viM->gpr[reg_dst] = val;
		set_flags_logic(viM, val);
		return false;
	}

	if (inst.sdss.op == 0x010) { // MOV reg, csr
		uint8_t val = viM->csr[reg_src];
		viM->gpr[reg_dst] = val;
		set_flags_logic(viM, val);
		return false;
	}
	if (inst.sdss.op == 0x090) { // MOV csr, reg
		viM->csr[reg_dst] = viM->gpr[reg_src];
		return false;
	}
	if (inst.sdss.op == 0x110) { // INCC
		uint16_t temp =
			(uint16_t)(viM->gpr[reg_src] + (viM->csr[0] & 0x01));
		viM->gpr[reg_dst] = (uint8_t)temp;
		set_flags_logic(viM, temp);
		return false;
	}
	if (inst.sdss.op == 0x190) { // DECB
		uint16_t temp = (uint16_t)(viM->gpr[reg_src] + 0xFF +
			(viM->csr[0] & 0x01));
		viM->gpr[reg_dst] = (uint8_t)temp;
		set_flags_logic(viM, temp);
		return false;
	}

	if (inst.op == 0x20 && inst.ds.op1 == 0x0) { // CMP
		uint16_t temp = (uint16_t)viM->gpr[reg_src] +
			(uint16_t)(~viM->gpr[reg_mod] & 0xFF) + 1;
		set_flags_sub(viM, viM->gpr[reg_src], viM->gpr[reg_mod], temp);
		return false;
	}
	if (inst.op == 0x20 && inst.ds.op1 == 0x1) { // CMA
		uint8_t temp = viM->gpr[reg_src] & viM->gpr[reg_mod];
		set_flags_logic(viM, temp);
		return false;
	}

	return true; // Unrecognized opcode 0
}

bool decode_execute(struct VirtualMachine *viM, uint16_t instruction)
{
	Instruction inst = { .raw = instruction };
	uint8_t reg_dst = inst.dst;
	uint8_t reg_src = inst.src;
	uint8_t reg_mod = inst.mod;
	uint8_t major_op = instruction & 0x0F;

	switch (major_op) {
	case 0x0:
		if (execute_op0(viM, instruction)) { goto illegal; }
		return false;

	case 0x1: { // 3-register ALU
		uint8_t alu_op = (instruction >> 4) & 0x07;
		uint16_t res = 0;
		switch (alu_op) {
		case 0x0: // SUB
			res = (uint16_t)viM->gpr[reg_src] +
				(uint16_t)(~viM->gpr[reg_mod] & 0xFF) + 1;
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_sub(viM, viM->gpr[reg_src],
				viM->gpr[reg_mod], res);
			break;
		case 0x1: // SBB
			res = (uint16_t)viM->gpr[reg_src] +
				(uint16_t)(~viM->gpr[reg_mod] & 0xFF) +
				(viM->csr[0] & 0x01);
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_sub(viM, viM->gpr[reg_src],
				viM->gpr[reg_mod], res);
			break;
		case 0x2: // ADD
			res = (uint16_t)viM->gpr[reg_src] + viM->gpr[reg_mod];
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_add(viM, viM->gpr[reg_src],
				viM->gpr[reg_mod], res);
			break;
		case 0x3: // ADC
			res = (uint16_t)viM->gpr[reg_src] + viM->gpr[reg_mod] +
				(viM->csr[0] & 0x01);
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_add(viM, viM->gpr[reg_src],
				viM->gpr[reg_mod], res);
			break;
		case 0x4: // AND / MOV
			res = viM->gpr[reg_src] & viM->gpr[reg_mod];
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x5: // OR
			res = viM->gpr[reg_src] | viM->gpr[reg_mod];
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x6: // NOR
			res = (uint8_t)~(
				viM->gpr[reg_src] | viM->gpr[reg_mod]);
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x7: // XOR
			res = viM->gpr[reg_src] ^ viM->gpr[reg_mod];
			viM->gpr[reg_dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		default:
			goto illegal;
		}
		return false;
	}

	case 0x2: { // Shifts
		uint8_t shift_op = (instruction >> 4) & 0x07;
		uint16_t res = 0;
		switch (shift_op) {
		case 0x0: // SLL reg
			res = (uint16_t)(viM->gpr[reg_src]
				<< viM->gpr[reg_mod]);
			break;
		case 0x1: // SRL reg
			res = (uint16_t)(viM->gpr[reg_src] >>
				viM->gpr[reg_mod]);
			break;
		case 0x2: // SRA reg
			res = (uint16_t)((uint8_t)((int8_t)viM->gpr[reg_src] >>
				viM->gpr[reg_mod]));
			break;
		case 0x4: // SLL imm
			res = (uint16_t)(viM->gpr[reg_src] << reg_mod);
			break;
		case 0x5: // SRL imm
			res = (uint16_t)(viM->gpr[reg_src] >> reg_mod);
			break;
		case 0x6: // SRA imm
			res = (uint16_t)((uint8_t)((int8_t)viM->gpr[reg_src] >>
				reg_mod));
			break;
		default:
			goto illegal;
		}
		viM->gpr[reg_dst] = (uint8_t)res;
		set_flags_logic(viM, res);
		return false;
	}

	case 0xA: { // LI
		int16_t imm = sign_extend(inst.li.imm, 8);
		viM->gpr[reg_dst] = (uint8_t)imm;
		set_flags_logic(viM, (uint16_t)imm);
		return false;
	}

	case 0xB: { // ADDI
		int16_t imm_add = sign_extend(
			((inst.addi.imm1 << 3) | inst.addi.imm0), 6);
		uint16_t res =
			(uint16_t)(viM->gpr[reg_src] + (uint8_t)imm_add);
		viM->gpr[reg_dst] = (uint8_t)res;
		set_flags_add(viM, viM->gpr[reg_src], (uint8_t)imm_add, res);
		return false;
	}

	case 0xC: { // SB
		uint8_t reg_base = (uint8_t)(inst.sb.base << 1);
		int16_t imm_sb = sign_extend(
			((inst.sb.offset1 << 4) | inst.sb.offset0), 7);
		uint16_t addr =
			(uint16_t)((viM->gpr[reg_base] |
					   (viM->gpr[reg_base + 1] << 8)) +
				imm_sb);
		memory_write(viM, addr, viM->gpr[reg_src]);
		return false;
	}

	case 0xD: { // LB
		uint8_t reg_base = (uint8_t)(inst.lb.base << 1);
		int16_t imm_lb = sign_extend(
			((inst.lb.offset1 << 4) | inst.lb.offset0), 7);
		uint16_t addr =
			(uint16_t)((viM->gpr[reg_base] |
					   (viM->gpr[reg_base + 1] << 8)) +
				imm_lb);
		uint8_t val = memory_read(viM, addr);
		viM->gpr[reg_dst] = val;
		set_flags_logic(viM, val);
		return false;
	}

	case 0xE: { // B rel
		int16_t imm_brel =
			(int16_t)(sign_extend(inst.branch.offset, 9) << 1);
		if (check_condition(viM, inst.branch.cond)) {
			viM->pc = (uint16_t)(viM->pc + imm_brel);
		}
		return false;
	}

	case 0xF: { // BL rel
		int16_t imm_brel =
			(int16_t)(sign_extend(inst.branch.offset, 9) << 1);
		if (check_condition(viM, inst.branch.cond)) {
			push_word(viM, viM->pc);
			viM->pc = (uint16_t)(viM->pc + imm_brel);
		}
		return false;
	}

	default:
		goto illegal;
	}

illegal:
	interrupt_pushtostack(viM);
	viM->pc = 0x0F04;
	printf("ERROR: illegal instruction: 0x%04x\n", instruction);
	return true;
}
