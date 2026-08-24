#pragma once
#include <stdint.h>

typedef union {
	uint16_t raw;

	struct {
		uint16_t op : 4;
		uint16_t dst : 4;
		uint16_t mod : 4;
		uint16_t hi : 4;
	};

	struct {
		uint16_t op : 4;
		uint16_t cond : 4;
		uint16_t mod : 4;
		uint16_t hi : 4;
	} cond;

	struct {
		uint16_t op : 4;
		uint16_t reg : 4;
		uint16_t bit : 1;
		uint16_t base : 3;
		uint16_t hi : 4;
	} mem;

	struct {
		uint16_t op : 4;
		uint16_t dst : 4;
		uint16_t imm : 8;
	} li;
} Instruction;

static inline uint8_t unpack_imm8(uint16_t instruction)
{
	uint8_t imm_hi = (uint8_t)((instruction >> 12) & 0x0F);
	uint8_t imm_lo = (uint8_t)((instruction >> 9) & 0x07);
	uint8_t imm_bit3 = (uint8_t)((instruction >> 8) & 0x01);
	return (uint8_t)((imm_hi << 4) | (imm_bit3 << 3) | imm_lo);
}

static inline uint8_t unpack_imm5(uint16_t instruction)
{
	uint8_t imm_hi = (uint8_t)((instruction >> 12) & 0x0F);
	uint8_t imm_lo = (uint8_t)((instruction >> 8) & 0x01);
	return (uint8_t)((imm_hi << 1) | imm_lo);
}

static inline uint16_t unpack_imm12(uint16_t instruction)
{
	uint16_t imm_hi = (uint16_t)((instruction >> 8) & 0xFF);
	uint16_t imm_bit3 = (uint16_t)((instruction >> 4) & 0x01);
	uint16_t imm_lo = (uint16_t)((instruction >> 5) & 0x07);
	return (uint16_t)((imm_hi << 4) | (imm_bit3 << 3) | imm_lo);
}
