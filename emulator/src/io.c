#include "io.h"
#include "graphics.h"
#include "memory.h"
#include "vm.h"
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

void trigger_interrupt(struct VirtualMachine *viM, uint8_t int_id)
{
	uint16_t stackp = (uint16_t)((viM->csr[0xF] << 8) | viM->csr[0xE]);

	memory_write(viM, stackp--, (uint8_t)(viM->pc >> 8));
	memory_write(viM, stackp--, (uint8_t)(viM->pc & 0xFF));
	memory_write(viM, stackp--, viM->csr[0]);

	viM->csr[0xF] = (uint8_t)(stackp >> 8);
	viM->csr[0xE] = (uint8_t)stackp;
	viM->csr[0] &= 0x7F;

	viM->wait_for_interrupt = false;

	uint16_t vbr_base = (uint16_t)((viM->csr[1] & 0xFE) << 8);
	if (viM->csr[1] == 0) { vbr_base = 0xFF00; }
	uint16_t vector_addr = (uint16_t)(vbr_base + ((int_id & 0x7F) << 1));
	viM->pc = (uint16_t)(memory_read(viM, vector_addr) |
		(memory_read(viM, (uint16_t)(vector_addr + 1)) << 8));
}

void check_and_dispatch_interrupts(
	struct VirtualMachine *viM, uint64_t ticks_ns)
{
	if ((viM->csr[0] & 0x80) == 0) { return; }

	uint8_t hw_ctrl = viM->hw_regs[HW_HW_CTRL - 0xFFF0];

	// 1. Timer Interrupt (Priority 1, ID 0x10)
	uint8_t hertz = viM->hw_regs[HW_TIMER_HZ - 0xFFF0];
	if (hertz > 0) {
		static uint64_t last_ticks = 0;
		uint64_t period_ns = 1000000000ULL / hertz;
		if (ticks_ns - last_ticks >= period_ns) {
			last_ticks = ticks_ns;
			trigger_interrupt(viM, 0x10);
			return;
		}
	}

	// 2. Keyboard Interrupt (Priority 2, ID 0x11, HW_CTRL bit 6)
	if ((hw_ctrl & 0x40) && viM->key_head != viM->key_tail) {
		trigger_interrupt(viM, 0x11);
		return;
	}

	// 3. UART RX Interrupt (Priority 3, ID 0x12, HW_CTRL bit 7)
	if ((hw_ctrl & 0x80) && viM->uart_head != viM->uart_tail) {
		trigger_interrupt(viM, 0x12);
		return;
	}

	// 4. Vsync Interrupt (Priority 4, ID 0x13, HW_CTRL bit 5)
	if (hw_ctrl & 0x20) {
		if (viM->vsync_pending) {
			viM->vsync_pending = false;
			trigger_interrupt(viM, 0x13);
			return;
		}
		if (!viM->graphics) {
			static uint64_t last_vsync_ticks = 0;
			uint64_t period_ns = 1000000000ULL / 60;
			if (ticks_ns - last_vsync_ticks >= period_ns) {
				last_vsync_ticks = ticks_ns;
				trigger_interrupt(viM, 0x13);
				return;
			}
		}
	}

	// 5. Hsync Interrupt (Priority 5, ID 0x14, HW_CTRL bit 4)
	if (hw_ctrl & 0x10) {
		static uint64_t last_hsync_ticks = 0;
		int total_lines = get_graphics_mode_height(viM);
		if (total_lines <= 0) { total_lines = 256; }
		uint64_t period_ns =
			1000000000ULL / (60ULL * (uint64_t)total_lines);
		if (ticks_ns - last_hsync_ticks >= period_ns) {
			last_hsync_ticks = ticks_ns;
			render_scanline(viM, viM->current_scanline);
			viM->current_scanline++;
			if (viM->current_scanline >= total_lines) {
				viM->current_scanline = 0;
				viM->vsync_pending = true;
			}
			trigger_interrupt(viM, 0x14);
			return;
		}
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
