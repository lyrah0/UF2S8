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
	int aux_carry = (viM->csr[0] >> 4) & 1;
	int overflow = (viM->csr[0] >> 3) & 1;
	int negative = (viM->csr[0] >> 2) & 1;
	int zero = (viM->csr[0] >> 1) & 1;
	int carry = viM->csr[0] & 1;

	printf("PC: 0x%04x\tinstruction: 0x%04x\tflags: I:%d A:%d V:%d N:%d " "Z:%d C:%d\n",
		(uint16_t)(viM->pc - 2), instruction, interrupt, aux_carry,
		overflow, negative, zero, carry);
	printf("r0:0x%02x r1:0x%02x r2:0x%02x r3:0x%02x r4:0x%02x r5:0x%02x " "r6:0x%02x r7:0x%02x\n",
		viM->gpr[0], viM->gpr[1], viM->gpr[2], viM->gpr[3],
		viM->gpr[4], viM->gpr[5], viM->gpr[6], viM->gpr[7]);
	printf("r8:0x%02x r9:0x%02x r10:0x%02x r11:0x%02x r12:0x%02x " "r13:0x%02x r14:0x%02x r15:0x%02x sp:0x%02x%02x\n",
		viM->gpr[8], viM->gpr[9], viM->gpr[10], viM->gpr[11],
		viM->gpr[12], viM->gpr[13], viM->gpr[14], viM->gpr[15],
		viM->csr[0xF], viM->csr[0xE]);
}

static inline bool check_condition(
	const struct VirtualMachine *viM, uint8_t cond)
{
	uint8_t flags = viM->csr[0];
	bool c = (flags & 0x01) != 0;
	bool z = (flags & 0x02) != 0;
	bool n = (flags & 0x04) != 0;
	bool v = (flags & 0x08) != 0;
	bool a = (flags & 0x10) != 0;

	switch (cond) {
	case 0: // ZS / EQ
		return z;
	case 1: // ZC / NE
		return (!z) != 0;
	case 2: // CS / HS
		return c;
	case 3: // CC / LO
		return (!c) != 0;
	case 4: // NS / MI
		return n;
	case 5: // NC / PL
		return (!n) != 0;
	case 6: // VS
		return v;
	case 7: // VC
		return (!v) != 0;
	case 8: // HI
		return (c && !z) != 0;
	case 9: // LS
		return (!c || z) != 0;
	case 10: // GE
		return n == v;
	case 11: // LT
		return n != v;
	case 12: // GT
		return (!z && (n == v)) != 0;
	case 13: // LE
		return (z || (n != v)) != 0;
	case 14: // AS
		return a;
	case 15: // AC
		return (!a) != 0;
	default:
		return true;
	}
}

static inline void set_flags_add(
	struct VirtualMachine *viM, uint8_t a, uint8_t b, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0x80;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	if (((a ^ res) & (b ^ res)) & 0x80) { flags |= 0x08; }
	if (((a & 0x0F) + (b & 0x0F)) > 0x0F) { flags |= 0x10; }
	viM->csr[0] = flags;
}

static inline void set_flags_adc(struct VirtualMachine *viM, uint8_t a,
	uint8_t b, uint8_t cin, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0x80;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	if (((a ^ res) & (b ^ res)) & 0x80) { flags |= 0x08; }
	if (((a & 0x0F) + (b & 0x0F) + cin) > 0x0F) { flags |= 0x10; }
	viM->csr[0] = flags;
}

static inline void set_flags_sub(
	struct VirtualMachine *viM, uint8_t a, uint8_t b, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0x80;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	if (((a ^ b) & (a ^ res)) & 0x80) { flags |= 0x08; }
	if ((a & 0x0F) < (b & 0x0F)) { flags |= 0x10; }
	viM->csr[0] = flags;
}

static inline void set_flags_sbb(struct VirtualMachine *viM, uint8_t a,
	uint8_t b, uint8_t bin, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0x80;
	if (res > 0xFF) { flags |= 0x01; }
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	if (((a ^ b) & (a ^ res)) & 0x80) { flags |= 0x08; }
	if ((a & 0x0F) < ((b & 0x0F) + bin)) { flags |= 0x10; }
	viM->csr[0] = flags;
}

static inline void set_flags_logic(struct VirtualMachine *viM, uint16_t res)
{
	uint8_t flags = viM->csr[0] & 0x99;
	flags &= (uint8_t)~0x06;
	if ((uint8_t)res == 0) { flags |= 0x02; }
	if (res & 0x80) { flags |= 0x04; }
	viM->csr[0] = flags;
}

static inline void push_word(struct VirtualMachine *viM, uint16_t val)
{
	uint16_t sp = (uint16_t)((viM->csr[0xF] << 8) | viM->csr[0xE]);
	memory_write(viM, sp--, (uint8_t)(val >> 8));
	memory_write(viM, sp--, (uint8_t)(val & 0xFF));
	viM->csr[0xF] = (uint8_t)(sp >> 8);
	viM->csr[0xE] = (uint8_t)sp;
}

static inline uint16_t pop_word(struct VirtualMachine *viM)
{
	uint16_t sp = (uint16_t)((viM->csr[0xF] << 8) | viM->csr[0xE]);
	uint8_t low = memory_read(viM, ++sp);
	uint8_t high = memory_read(viM, ++sp);
	viM->csr[0xF] = (uint8_t)(sp >> 8);
	viM->csr[0xE] = (uint8_t)sp;
	return (uint16_t)(low | (high << 8));
}

static inline uint16_t get_addr_reg(
	const struct VirtualMachine *viM, uint8_t base)
{
	uint8_t idx = (uint8_t)((base & 0x07) * 2);
	return (uint16_t)(viM->gpr[idx] | (viM->gpr[idx + 1] << 8));
}

static bool execute_op0(struct VirtualMachine *viM, uint16_t instruction)
{
	if (instruction == 0x0000) { // NOP
		return false;
	}
	if (instruction == 0x0010) { // RET
		viM->pc = pop_word(viM);
		return false;
	}
	if (instruction == 0x0020) { // WFI
		viM->wait_for_interrupt = true;
		return false;
	}
	if (instruction == 0x0030) { // RETI
		uint16_t sp = (uint16_t)((viM->csr[0xF] << 8) | viM->csr[0xE]);
		viM->csr[0] = memory_read(viM, ++sp);
		uint8_t low = memory_read(viM, ++sp);
		uint8_t high = memory_read(viM, ++sp);
		viM->pc = (uint16_t)(low | (high << 8));
		viM->csr[0xF] = (uint8_t)(sp >> 8);
		viM->csr[0xE] = (uint8_t)sp;
		return false;
	}

	uint8_t hi_byte = (uint8_t)((instruction >> 8) & 0xFF);
	uint8_t hi_nibble = (uint8_t)((instruction >> 12) & 0x0F);
	uint8_t mod_nibble = (uint8_t)((instruction >> 8) & 0x0F);
	uint8_t dst_nibble = (uint8_t)((instruction >> 4) & 0x0F);

	if (hi_byte == 0x11) { // SWI
		uint8_t swi_id = viM->gpr[dst_nibble] & 0x7F;
		trigger_interrupt(viM, swi_id);
		if (swi_id == 0) { viM->running = false; }
		if (swi_id == 1) { viM->debug_mode = true; }
		return false;
	}
	if (hi_byte == 0x12) { // PUSH
		uint16_t sp = (uint16_t)((viM->csr[0xF] << 8) | viM->csr[0xE]);
		memory_write(viM, sp--, viM->gpr[dst_nibble]);
		viM->csr[0xF] = (uint8_t)(sp >> 8);
		viM->csr[0xE] = (uint8_t)sp;
		return false;
	}
	if (hi_byte == 0x13) { // POP
		uint16_t sp = (uint16_t)((viM->csr[0xF] << 8) | viM->csr[0xE]);
		uint8_t val = memory_read(viM, ++sp);
		viM->csr[0xF] = (uint8_t)(sp >> 8);
		viM->csr[0xE] = (uint8_t)sp;
		viM->gpr[dst_nibble] = val;
		set_flags_logic(viM, val);
		return false;
	}
	if ((instruction & 0xFF1F) == 0x1D00) { // B reg uncond
		uint8_t b = (uint8_t)((instruction >> 5) & 0x07);
		viM->pc = get_addr_reg(viM, b);
		return false;
	}
	if ((instruction & 0xFF1F) == 0x1D10) { // BL reg uncond
		uint8_t b = (uint8_t)((instruction >> 5) & 0x07);
		push_word(viM, viM->pc);
		viM->pc = get_addr_reg(viM, b);
		return false;
	}
	if ((instruction & 0xFF1F) == 0x1E00) { // BST flags
		uint8_t imm = (uint8_t)((instruction >> 5) & 0x07);
		viM->csr[0] |= (uint8_t)(1 << imm);
		return false;
	}
	if ((instruction & 0xFF1F) == 0x1E10) { // BIC flags
		uint8_t imm = (uint8_t)((instruction >> 5) & 0x07);
		viM->csr[0] &= (uint8_t)~(1 << imm);
		return false;
	}
	if ((instruction & 0xFF1F) == 0x1F00) { // BTS flags
		uint8_t imm = (uint8_t)((instruction >> 5) & 0x07);
		uint8_t bit_val = (uint8_t)((viM->csr[0] >> imm) & 1);
		set_flags_logic(viM, bit_val);
		return false;
	}
	if ((instruction & 0xF10F) == 0x2000) { // B cond reg
		uint8_t b = (uint8_t)((instruction >> 9) & 0x07);
		if (check_condition(viM, dst_nibble)) {
			viM->pc = get_addr_reg(viM, b);
		}
		return false;
	}
	if ((instruction & 0xF10F) == 0x2100) { // BL cond reg
		uint8_t b = (uint8_t)((instruction >> 9) & 0x07);
		if (check_condition(viM, dst_nibble)) {
			push_word(viM, viM->pc);
			viM->pc = get_addr_reg(viM, b);
		}
		return false;
	}
	if (hi_nibble == 0x3) { // MOV rd, rs
		viM->gpr[dst_nibble] = viM->gpr[mod_nibble];
		set_flags_logic(viM, viM->gpr[dst_nibble]);
		return false;
	}
	if ((instruction & 0xF10F) == 0x4000) { // BST rd, imm
		uint8_t imm = (uint8_t)((instruction >> 9) & 0x07);
		viM->gpr[dst_nibble] |= (uint8_t)(1 << imm);
		set_flags_logic(viM, viM->gpr[dst_nibble]);
		return false;
	}
	if ((instruction & 0xF10F) == 0x4100) { // BIC rd, imm
		uint8_t imm = (uint8_t)((instruction >> 9) & 0x07);
		viM->gpr[dst_nibble] &= (uint8_t)~(1 << imm);
		set_flags_logic(viM, viM->gpr[dst_nibble]);
		return false;
	}
	if (hi_nibble == 0x6) { // NOT
		viM->gpr[dst_nibble] = (uint8_t)~viM->gpr[mod_nibble];
		set_flags_logic(viM, viM->gpr[dst_nibble]);
		return false;
	}
	if (hi_nibble == 0x7) { // NEG
		uint16_t res = (uint16_t)(0 - viM->gpr[mod_nibble]);
		viM->gpr[dst_nibble] = (uint8_t)res;
		set_flags_sub(viM, 0, viM->gpr[mod_nibble], res);
		return false;
	}
	if (hi_nibble == 0x8) { // INC
		uint16_t res = (uint16_t)(viM->gpr[mod_nibble] + 1);
		viM->gpr[dst_nibble] = (uint8_t)res;
		set_flags_add(viM, viM->gpr[mod_nibble], 1, res);
		return false;
	}
	if (hi_nibble == 0x9) { // DEC
		uint16_t res = (uint16_t)(viM->gpr[mod_nibble] - 1);
		viM->gpr[dst_nibble] = (uint8_t)res;
		set_flags_sub(viM, viM->gpr[mod_nibble], 1, res);
		return false;
	}
	if (hi_nibble == 0xA) { // MOV rd, csr
		viM->gpr[dst_nibble] = viM->csr[mod_nibble];
		set_flags_logic(viM, viM->gpr[dst_nibble]);
		return false;
	}
	if (hi_nibble == 0xB) { // MOV csr, rs
		viM->csr[dst_nibble] = viM->gpr[mod_nibble];
		return false;
	}
	if (hi_nibble == 0xC) { // INCC
		uint8_t c = viM->csr[0] & 0x01;
		uint16_t res = (uint16_t)(viM->gpr[mod_nibble] + c);
		viM->gpr[dst_nibble] = (uint8_t)res;
		set_flags_adc(viM, viM->gpr[mod_nibble], 0, c, res);
		return false;
	}
	if (hi_nibble == 0xD) { // DECB
		uint16_t temp = (uint16_t)(viM->gpr[mod_nibble] + 0xFF +
			(viM->csr[0] & 0x01));
		viM->gpr[dst_nibble] = (uint8_t)temp;
		set_flags_logic(viM, temp);
		return false;
	}
	if (hi_nibble == 0xE) { // CMP
		uint16_t temp = (uint16_t)viM->gpr[mod_nibble] +
			(uint16_t)(~viM->gpr[dst_nibble] & 0xFF) + 1;
		set_flags_sub(
			viM, viM->gpr[mod_nibble], viM->gpr[dst_nibble], temp);
		return false;
	}
	if (hi_nibble == 0xF) { // CMA
		uint8_t temp = viM->gpr[mod_nibble] & viM->gpr[dst_nibble];
		set_flags_logic(viM, temp);
		return false;
	}

	return true;
}

bool decode_execute(struct VirtualMachine *viM, uint16_t instruction)
{
	uint8_t major_op = instruction & 0x0F;
	uint8_t dst = (uint8_t)((instruction >> 4) & 0x0F);
	uint8_t mod = (uint8_t)((instruction >> 8) & 0x0F);

	switch (major_op) {
	case 0x0:
		if (execute_op0(viM, instruction)) { goto illegal; }
		return false;

	case 0x1: { // ALU / Shift / Bit operations
		uint8_t alu_op = (uint8_t)((instruction >> 12) & 0x0F);
		uint16_t res = 0;
		switch (alu_op) {
		case 0x0: // SUB
			res = (uint16_t)(viM->gpr[dst] - viM->gpr[mod]);
			set_flags_sub(viM, viM->gpr[dst], viM->gpr[mod], res);
			viM->gpr[dst] = (uint8_t)res;
			break;
		case 0x1: { // SBB
			uint8_t b = (viM->csr[0] & 0x01) ? 0 : 1;
			res = (uint16_t)(viM->gpr[dst] - viM->gpr[mod] - b);
			set_flags_sbb(
				viM, viM->gpr[dst], viM->gpr[mod], b, res);
			viM->gpr[dst] = (uint8_t)res;
			break;
		}
		case 0x2: // ADD
			res = (uint16_t)(viM->gpr[dst] + viM->gpr[mod]);
			set_flags_add(viM, viM->gpr[dst], viM->gpr[mod], res);
			viM->gpr[dst] = (uint8_t)res;
			break;
		case 0x3: { // ADC
			uint8_t c = viM->csr[0] & 0x01;
			res = (uint16_t)(viM->gpr[dst] + viM->gpr[mod] + c);
			set_flags_adc(
				viM, viM->gpr[dst], viM->gpr[mod], c, res);
			viM->gpr[dst] = (uint8_t)res;
			break;
		}
		case 0x4: // AND
			res = viM->gpr[dst] & viM->gpr[mod];
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x5: // ANDN
			res = viM->gpr[dst] & (uint8_t)~viM->gpr[mod];
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x6: // OR
			res = viM->gpr[dst] | viM->gpr[mod];
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x7: // ORN
			res = viM->gpr[dst] | (uint8_t)~viM->gpr[mod];
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x8: // NOR
			res = (uint8_t)~(viM->gpr[dst] | viM->gpr[mod]);
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0x9: // XOR
			res = viM->gpr[dst] ^ viM->gpr[mod];
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0xA: // SLL reg
			res = (uint16_t)(viM->gpr[dst]
				<< (viM->gpr[mod] & 0x07));
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0xB: // SRL reg
			res = (uint16_t)(viM->gpr[dst] >>
				(viM->gpr[mod] & 0x07));
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0xC: // SRA reg
			res = (uint16_t)((uint8_t)((int8_t)viM->gpr[dst] >>
				(viM->gpr[mod] & 0x07)));
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		case 0xD: { // SLL imm
			uint8_t imm = (uint8_t)((instruction >> 9) & 0x07);
			res = (uint16_t)(viM->gpr[dst] << imm);
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		}
		case 0xE: { // SRL imm
			uint8_t imm = (uint8_t)((instruction >> 9) & 0x07);
			res = (uint16_t)(viM->gpr[dst] >> imm);
			viM->gpr[dst] = (uint8_t)res;
			set_flags_logic(viM, res);
			break;
		}
		case 0xF: {
			uint8_t imm = (uint8_t)((instruction >> 9) & 0x07);
			if (instruction & 0x0100) { // BTS imm
				uint8_t bit_val =
					(uint8_t)((viM->gpr[dst] >> imm) & 1);
				set_flags_logic(viM, bit_val);
			} else { // SRA imm
				res = (uint16_t)((
					uint8_t)((int8_t)viM->gpr[dst] >>
					imm));
				viM->gpr[dst] = (uint8_t)res;
				set_flags_logic(viM, res);
			}
			break;
		}
		default:
			goto illegal;
		}
		return false;
	}

	case 0x2: { // Register Bit Operations
		uint8_t op2 = (uint8_t)((instruction >> 12) & 0x0F);
		uint8_t bit_pos = viM->gpr[mod] & 0x07;
		if (op2 == 0x0) { // BST reg
			viM->gpr[dst] |= (uint8_t)(1 << bit_pos);
			set_flags_logic(viM, viM->gpr[dst]);
		} else if (op2 == 0x1) { // BIC reg
			viM->gpr[dst] &= (uint8_t)~(1 << bit_pos);
			set_flags_logic(viM, viM->gpr[dst]);
		} else if (op2 == 0x2) { // BTS reg
			uint8_t bit_val =
				(uint8_t)((viM->gpr[dst] >> bit_pos) & 1);
			set_flags_logic(viM, bit_val);
		} else {
			goto illegal;
		}
		return false;
	}

	case 0x8: { // ADD imm
		uint8_t imm = unpack_imm8(instruction);
		uint16_t res = (uint16_t)(viM->gpr[dst] + imm);
		set_flags_add(viM, viM->gpr[dst], imm, res);
		viM->gpr[dst] = (uint8_t)res;
		return false;
	}

	case 0x9: { // LI
		uint8_t imm = unpack_imm8(instruction);
		viM->gpr[dst] = imm;
		set_flags_logic(viM, imm);
		return false;
	}

	case 0xA: { // SB
		uint8_t off = unpack_imm5(instruction);
		int16_t imm = sign_extend((int16_t)off, 5);
		uint8_t b = (uint8_t)((instruction >> 9) & 0x07);
		uint16_t addr = (uint16_t)(get_addr_reg(viM, b) + imm);
		memory_write(viM, addr, viM->gpr[dst]);
		return false;
	}

	case 0xB: { // LB
		uint8_t off = unpack_imm5(instruction);
		int16_t imm = sign_extend((int16_t)off, 5);
		uint8_t b = (uint8_t)((instruction >> 9) & 0x07);
		uint16_t addr = (uint16_t)(get_addr_reg(viM, b) + imm);
		uint8_t val = memory_read(viM, addr);
		viM->gpr[dst] = val;
		set_flags_logic(viM, val);
		return false;
	}

	case 0xC: { // B cond rel
		uint8_t off = unpack_imm8(instruction);
		int16_t imm = (int16_t)(sign_extend((int16_t)off, 8) << 1);
		if (check_condition(viM, dst)) {
			viM->pc = (uint16_t)(viM->pc + imm);
		}
		return false;
	}

	case 0xD: { // BL cond rel
		uint8_t off = unpack_imm8(instruction);
		int16_t imm = (int16_t)(sign_extend((int16_t)off, 8) << 1);
		if (check_condition(viM, dst)) {
			push_word(viM, viM->pc);
			viM->pc = (uint16_t)(viM->pc + imm);
		}
		return false;
	}

	case 0xE: { // B uncond rel
		uint16_t off = unpack_imm12(instruction);
		int16_t imm = (int16_t)(sign_extend((int16_t)off, 12) << 1);
		viM->pc = (uint16_t)(viM->pc + imm);
		return false;
	}

	case 0xF: { // BL uncond rel
		uint16_t off = unpack_imm12(instruction);
		int16_t imm = (int16_t)(sign_extend((int16_t)off, 12) << 1);
		push_word(viM, viM->pc);
		viM->pc = (uint16_t)(viM->pc + imm);
		return false;
	}

	default:
		goto illegal;
	}

illegal:
	trigger_interrupt(viM, 0x02);
	printf("ERROR: illegal instruction: 0x%04x at PC: 0x%04x\n",
		instruction, (uint16_t)(viM->pc - 2));
	return true;
}
