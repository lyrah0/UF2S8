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

void interrupt_timer(struct VirtualMachine *viM, const unsigned int *timer)
{
	uint16_t timer_mult = viM->memory[0xFEFE] << 8 | 0xFF;
	if (!(*timer % timer_mult == 0 && (viM->csr[0] >> 7) == 1)) { return; }

	uint8_t hardware_control = viM->memory[0xFEFF];
	bool timer_enabled = (hardware_control & 0x1) != 0;

	if (timer_enabled) {
		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF06;
		viM->pc = viM->memory[vector_addr] |
			viM->memory[vector_addr + 1] << 8;
	}
}

void interrupt_input(struct VirtualMachine *viM)
{
	// Only interrupt if interrupts are enabled, the keyboard interrupt is enabled, and there are keys in the buffer.
	if ((viM->csr[0] >> 7) == 1 &&
		(viM->memory[HW_HARDWARE_CONTROL] & 0x02) &&
		viM->key_head != viM->key_tail) {
		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF06;
		viM->pc = viM->memory[vector_addr] |
			viM->memory[vector_addr + 1] << 8;
	}
}
