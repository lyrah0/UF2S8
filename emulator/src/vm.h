#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

enum {
	GFX_MODE0_RES = 200,
	GFX_MODE1_RES = 180,
	GFX_MODE2_RES = 160,
	GFX_MODE3_RES = 140,
	GFX_MODE4_RES = 280,
	GFX_MODE5_RES = 240,
	GFX_MODE6_RES = 200,
	GFX_MODE7_RES = 400,
	GFX_MODE8_RES = 280,
	GFX_MODE9_RES = 200,
	GFX_MODE10_RES = 570,
	GFX_MODE11_RES = 400,
	GFX_MODE12_RES = 280,
	GFX_MODE13_RES = 200,
	DEFAULT_DISPLAY_HEIGHT = 1080,
	HW_BLIT_SRC_X_L = 0xFEC0,
	HW_BLIT_SRC_X_H = 0xFEC1,
	HW_BLIT_SRC_Y_L = 0xFEC2,
	HW_BLIT_SRC_Y_H = 0xFEC3,
	HW_BLIT_DST_X_L = 0xFEC4,
	HW_BLIT_DST_X_H = 0xFEC5,
	HW_BLIT_DST_Y_L = 0xFEC6,
	HW_BLIT_DST_Y_H = 0xFEC7,
	HW_BLIT_CLIP_X_MIN_L = 0xFEC8,
	HW_BLIT_CLIP_X_MIN_H = 0xFEC9,
	HW_BLIT_CLIP_Y_MIN_L = 0xFECA,
	HW_BLIT_CLIP_Y_MIN_H = 0xFECB,
	HW_BLIT_CLIP_X_MAX_L = 0xFECC,
	HW_BLIT_CLIP_X_MAX_H = 0xFECD,
	HW_BLIT_CLIP_Y_MAX_L = 0xFECE,
	HW_BLIT_CLIP_Y_MAX_H = 0xFECF,
	HW_BLIT_SRC_STRIDE_L = 0xFED0,
	HW_BLIT_SRC_STRIDE_H = 0xFED1,
	HW_BLIT_DST_STRIDE_L = 0xFED2,
	HW_BLIT_DST_STRIDE_H = 0xFED3,
	HW_BLIT_WIDTH_L = 0xFED4,
	HW_BLIT_WIDTH_H = 0xFED5,
	HW_BLIT_HEIGHT_L = 0xFED6,
	HW_BLIT_HEIGHT_H = 0xFED7,
	HW_BLIT_ALPHA = 0xFEDC,
	HW_BLIT_FLAGS = 0xFEDD,
	HW_BLIT_COLOR = 0xFEDE,
	HW_BLIT_CMD = 0xFEDF,
	HW_GFX_ADDR_L = 0xFEE0,
	HW_GFX_ADDR_H = 0xFEE1,
	HW_GFX_DATA = 0xFEEE,
	HW_GFX_CONTROL = 0xFEEF,
	HW_PALETTE_START = 0xFFF0,
	HW_TERMINAL_OUT = 0xFEF0,
	HW_KEYBOARD_DATA = 0xFEF1,
	HW_KEYBOARD_STATUS = 0xFEF2,
	HW_TIMER_MULT = 0xFEFE,
	HW_HARDWARE_CONTROL = 0xFEFF
};

enum { MAX_MEMORY = 1 << 16, MAX_BREAKPOINTS = 1 << 4, VRAM_SIZE = 1 << 16 };

struct VirtualMachine {
	uint8_t memory[MAX_MEMORY];
	uint8_t gpr[8];
	uint8_t csr[8];
	uint16_t pc;
	uint16_t breakpoint[MAX_BREAKPOINTS];
	int bp_count;
	volatile bool running;
	bool debug_mode;
	bool memory_dump;
	bool graphics;
	bool print_stats;
	bool wait_for_interrupt;
	SDL_Thread *cpu_thread;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *textures[14];
	SDL_Palette *sdl_palette;
	uint8_t vram[VRAM_SIZE];
	uint16_t vram_ptr;
	uint8_t processed_vram[912 * 570];
	uint16_t key_buffer[64];
	int key_head;
	int key_tail;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline int16_t sign_extend(uint16_t value, uint8_t bits)
{
	uint16_t shift = 16 - bits;
	return (int16_t)((int16_t)(value << shift) >> shift);
}
