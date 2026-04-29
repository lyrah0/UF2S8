#pragma once
#include <stdint.h>
#include <stdbool.h>

enum { MAX_MEMORY = 1 << 16, MAX_BREAKPOINTS = 1 << 4 };

struct VirtualMachine {
	uint8_t memory[MAX_MEMORY];
	uint8_t gpr[8];
	uint8_t csr[8];
	uint16_t pc;
	uint16_t breakpoint[MAX_BREAKPOINTS];
	int bp_count;
	bool running;
	bool debug_mode;
	bool memory_dump;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline int16_t sign_extend(uint16_t value, uint8_t bits)
{
	uint16_t shift = 16 - bits;
	return (int16_t)((int16_t)(value << shift) >> shift);
}
