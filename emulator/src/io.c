#include "io.h"
#include "memory.h"
#include "vm.h"
#include <stdint.h>

void interrupt_pushtostack(struct VirtualMachine *viM)
{
	uint16_t stackp = viM->csr[7] << 8 | viM->csr[6];

	memory_write(viM, stackp--, viM->pc >> 8);
	memory_write(viM, stackp--, viM->pc);
	memory_write(viM, stackp--, viM->csr[0]);

	viM->csr[7] = stackp >> 8;
	viM->csr[6] = stackp;
	viM->csr[0] &= 0x7F;
}

void interrupt_timer(struct VirtualMachine *viM, const uint64_t ticks_ns)
{
	uint8_t hertz = memory_read(viM, HW_TIMER_HZ);

	if ((viM->csr[0] & 0x80) == 0 ||
		(memory_read(viM, HW_HW_CTRL) & 0x01) == 0 || hertz == 0) {
		return;
	}

	static uint64_t last_ticks = 0;
	uint64_t period_ns = 1000000000ULL / hertz;

	if (ticks_ns - last_ticks < period_ns) { return; }
	last_ticks = ticks_ns;

	viM->wait_for_interrupt = false;

	interrupt_pushtostack(viM);

	uint16_t vector_addr = 0xFF20;
	viM->pc = memory_read(viM, vector_addr) |
		memory_read(viM, vector_addr + 1) << 8;
}

void interrupt_input(struct VirtualMachine *viM)
{
	// Only interrupt if interrupts are enabled, the keyboard interrupt is enabled, and there are keys in the buffer.
	if ((viM->csr[0] >> 7) == 1 && (memory_read(viM, HW_HW_CTRL) & 0x02) &&
		viM->key_head != viM->key_tail) {
		viM->wait_for_interrupt = false;
		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF22;
		viM->pc = memory_read(viM, vector_addr) |
			memory_read(viM, vector_addr + 1) << 8;
	}
}
