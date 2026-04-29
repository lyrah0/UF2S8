#include "io.h"
#include "vm.h"
#include <stdint.h>

void interrupt_pushtostack(struct VirtualMachine *viM)
{
	uint16_t stackp = viM->csr[7] << 8 | viM->csr[6];

	viM->memory[stackp--] = viM->pc >> 8;
	viM->memory[stackp--] = viM->pc;
	viM->memory[stackp--] = viM->csr[0];

	viM->csr[7] = stackp >> 8;
	viM->csr[6] = stackp;
	viM->csr[0] &= 0x7F;
}

void interrupt_timer(struct VirtualMachine *viM, const uint64_t ticks_ns)
{
	uint8_t hertz = viM->memory[HW_TIMER_MULT];

	if ((viM->csr[0] & 0x80) == 0 ||
		(viM->memory[HW_HARDWARE_CONTROL] & 0x01) == 0 || hertz == 0) {
		return;
	}

	static uint64_t last_ticks = 0;
	uint64_t period_ns = 1000000000ULL / hertz;

	if (ticks_ns - last_ticks < period_ns) { return; }
	last_ticks = ticks_ns;

	viM->wait_for_interrupt = false;

	interrupt_pushtostack(viM);

	uint16_t vector_addr = 0xFF04;
	viM->pc = viM->memory[vector_addr] | viM->memory[vector_addr + 1] << 8;
}

void interrupt_input(struct VirtualMachine *viM)
{
	// Only interrupt if interrupts are enabled, the keyboard interrupt is enabled, and there are keys in the buffer.
	if ((viM->csr[0] >> 7) == 1 &&
		(viM->memory[HW_HARDWARE_CONTROL] & 0x02) &&
		viM->key_head != viM->key_tail) {
		viM->wait_for_interrupt = false;
		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF06;
		viM->pc = viM->memory[vector_addr] |
			viM->memory[vector_addr + 1] << 8;
	}
}
