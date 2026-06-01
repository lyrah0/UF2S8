#include "io.h"
#include "memory.h"
#include "vm.h"
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

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
void interrupt_uart(struct VirtualMachine *viM)
{
	// Only interrupt if interrupts are enabled, the UART RX interrupt is enabled, and there is data in the buffer.
	if ((viM->csr[0] & 0x80) && (memory_read(viM, HW_UART_CTRL) & 0x01) &&
		viM->uart_head != viM->uart_tail) {
		viM->wait_for_interrupt = false;
		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF24;
		viM->pc = memory_read(viM, vector_addr) |
			memory_read(viM, vector_addr + 1) << 8;
	}
}

struct TerminalState {
	struct termios orig;
	int flags;
	bool initialized;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static struct TerminalState s_term_state = { .initialized = false };

static void restore_terminal_mode(void)
{
	if (s_term_state.initialized) {
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &s_term_state.orig);
		(void)fcntl(STDIN_FILENO, F_SETFL, s_term_state.flags);
	}
}

void handle_uart_events(struct VirtualMachine *viM)
{
	static bool is_raw = false;

	if (!s_term_state.initialized) {
		(void)tcgetattr(STDIN_FILENO, &s_term_state.orig);
		s_term_state.flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		(void)atexit(restore_terminal_mode);
		s_term_state.initialized = true;
	}

	if (viM->debug_mode && is_raw) {
		restore_terminal_mode();
		is_raw = false;
	} else if (!viM->debug_mode && !is_raw) {
		struct termios raw = s_term_state.orig;
		raw.c_lflag &= ~(ECHO | ICANON);
		(void)fcntl(STDIN_FILENO, F_SETFL,
			s_term_state.flags | O_NONBLOCK);
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
		is_raw = true;
	}

	if (viM->debug_mode) { return; }

	uint8_t character = 0;
	while (read(STDIN_FILENO, &character, 1) > 0) {
		int next = (viM->uart_tail + 1) % 64;
		if (next != viM->uart_head) {
			viM->uart_buffer[viM->uart_tail] = character;
			viM->uart_tail = next;
		}
	}
}
