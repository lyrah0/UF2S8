#include "graphics.h"
#include "vm.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void handle_graphics_events(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			viM->running = false;
		} else if (event.type == SDL_EVENT_KEY_DOWN ||
			event.type == SDL_EVENT_KEY_UP) {
			bool released = (event.type == SDL_EVENT_KEY_UP);
			uint8_t scancode = (uint8_t)event.key.scancode;

			int next = (viM->key_tail + 1) % 64;
			if (next != viM->key_head) {
				viM->key_buffer[viM->key_tail] =
					((uint16_t)released << 8) | scancode;
				viM->key_tail = next;
			}
		}
	}
}

struct GraphicsModeInfo {
	int width;
	int height;
	int bpp;
	int vram_bytes;
};

static const struct GraphicsModeInfo GFX_MODES[11] = {
	{ 256, 256, 8, 65536 }, // 0: 8bpp 256x256 64KiB
	{ 256, 256, 4, 32768 }, // 1: 4bpp 256x256 32KiB
	{ 256, 256, 2, 16384 }, // 2: 2bpp 256x256 16KiB
	{ 256, 256, 1, 8192  }, // 3: 1bpp 256x256 8KiB
	{ 352, 352, 4, 61952 }, // 4: 4bpp 352x352 61952B
	{ 496, 496, 2, 61504 }, // 5: 2bpp 496x496 61504B
	{ 704, 704, 1, 63504 }, // 6: 1bpp 704x704 63504B
	{ 304, 208, 8, 63232 }, // 7: 8bpp 304x208 63232B
	{ 448, 288, 4, 64512 }, // 8: 4bpp 448x288 64512B
	{ 624, 416, 2, 64896 }, // 9: 2bpp 624x416 64896B
	{ 896, 576, 1, 64512 }, // 10: 1bpp 896x576 64512B
};

void render_scanline(struct VirtualMachine *viM, int y)
{
	uint8_t mode = viM->hw_regs[HW_HW_CTRL - 0xFFF0] & 0x0F;
	if (mode > 10) { mode = 0; }

	const struct GraphicsModeInfo *minfo = &GFX_MODES[mode];
	const int res_w = minfo->width;
	const int res_h = minfo->height;
	const int bpp = minfo->bpp;

	if (y < 0 || y >= res_h) { return; }

	if (bpp == 8) {
		memcpy(&viM->processed_vram[(ptrdiff_t)y * res_w],
			&viM->vram[(ptrdiff_t)y * res_w], (size_t)res_w);
	} else if (bpp == 4) {
		int vram_row_bytes = res_w / 2;
		const uint8_t *src = &viM->vram[(ptrdiff_t)y * vram_row_bytes];
		uint8_t *dst = &viM->processed_vram[(ptrdiff_t)y * res_w];
		for (int x = 0; x < vram_row_bytes; x++) {
			uint8_t vbyte = src[x];
			uint8_t p0 = (vbyte >> 4) & 0x0F;
			uint8_t p1 = vbyte & 0x0F;
			dst[(ptrdiff_t)x * 2] = viM->vram[0xFFF0 + p0];
			dst[(ptrdiff_t)x * 2 + 1] = viM->vram[0xFFF0 + p1];
		}
	} else if (bpp == 2) {
		int vram_row_bytes = res_w / 4;
		const uint8_t *src = &viM->vram[(ptrdiff_t)y * vram_row_bytes];
		uint8_t *dst = &viM->processed_vram[(ptrdiff_t)y * res_w];
		for (int x = 0; x < vram_row_bytes; x++) {
			uint8_t vbyte = src[x];
			for (int j = 0; j < 4; j++) {
				uint8_t p = (uint8_t)((vbyte >> ((3 - j) * 2)) & 0x03);
				dst[(ptrdiff_t)x * 4 + j] = viM->vram[0xFFF0 + p];
			}
		}
	} else if (bpp == 1) {
		int vram_row_bytes = res_w / 8;
		const uint8_t *src = &viM->vram[(ptrdiff_t)y * vram_row_bytes];
		uint8_t *dst = &viM->processed_vram[(ptrdiff_t)y * res_w];
		for (int x = 0; x < vram_row_bytes; x++) {
			uint8_t vbyte = src[x];
			for (int j = 0; j < 8; j++) {
				uint8_t p = (uint8_t)((vbyte >> (7 - j)) & 0x01);
				dst[(ptrdiff_t)x * 8 + j] = viM->vram[0xFFF0 + p];
			}
		}
	}
}

__attribute__((pure)) int get_graphics_mode_height(
	const struct VirtualMachine *viM)
{
	uint8_t mode = viM->hw_regs[HW_HW_CTRL - 0xFFF0] & 0x0F;
	if (mode > 10) { mode = 0; }
	return GFX_MODES[mode].height;
}

void render_graphics_frame(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }

	uint8_t mode = viM->hw_regs[HW_HW_CTRL - 0xFFF0] & 0x0F;
	if (mode > 10) { mode = 0; }

	const struct GraphicsModeInfo *minfo = &GFX_MODES[mode];
	const int res_w = minfo->width;
	const int res_h = minfo->height;

	if (viM->current_res_w != res_w || viM->current_res_h != res_h) {
		if (viM->dynamic_texture) {
			SDL_DestroyTexture(viM->dynamic_texture);
		}
		viM->dynamic_texture = SDL_CreateTexture(viM->renderer,
			SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STREAMING, res_w, res_h);
		if (viM->dynamic_texture) {
			SDL_SetTexturePalette(viM->dynamic_texture, viM->sdl_palette);
			SDL_SetTextureScaleMode(viM->dynamic_texture, SDL_SCALEMODE_NEAREST);
		}
		SDL_SetRenderLogicalPresentation(viM->renderer, res_w, res_h,
			SDL_LOGICAL_PRESENTATION_LETTERBOX);
		viM->current_res_w = res_w;
		viM->current_res_h = res_h;
	}

	// If Hsync raster interrupt is not enabled, render full frame now
	if ((viM->hw_regs[HW_HW_CTRL - 0xFFF0] & 0x10) == 0) {
		for (int y = 0; y < res_h; y++) {
			render_scanline(viM, y);
		}
	}

	SDL_UpdateTexture(viM->dynamic_texture, nullptr, viM->processed_vram, res_w);
	SDL_RenderClear(viM->renderer);
	SDL_RenderTexture(
		viM->renderer, viM->dynamic_texture, nullptr, nullptr);
	SDL_RenderPresent(viM->renderer);
	viM->vsync_pending = true;
}

bool init_sdl(struct VirtualMachine *viM) // NOLINT
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init Error: %s\n", SDL_GetError());
		return true;
	}

	viM->window = SDL_CreateWindow(
		"UF2S8 Emulator", 512, 512, SDL_WINDOW_RESIZABLE);
	if (!viM->window) {
		printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
		SDL_Quit();
		return true;
	}

	viM->renderer = SDL_CreateRenderer(viM->window, nullptr);
	if (!viM->renderer) {
		printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	SDL_SetRenderVSync(viM->renderer, 1);

	viM->sdl_palette = SDL_CreatePalette(256);
	if (!viM->sdl_palette) {
		printf("SDL_CreatePalette Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	SDL_Color s_rgb332[256];
	for (int i = 0; i < 256; i++) {
		s_rgb332[i].r = (uint8_t)((i >> 5) * 255 / 7);
		s_rgb332[i].g = (uint8_t)(((i >> 2) & 0x07) * 255 / 7);
		s_rgb332[i].b = (uint8_t)((i & 0x03) * 255 / 3);
		s_rgb332[i].a = 255;
	}
	SDL_SetPaletteColors(viM->sdl_palette, s_rgb332, 0, 256);

	viM->dynamic_texture = SDL_CreateTexture(viM->renderer,
		SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STREAMING, 256, 256);
	if (!viM->dynamic_texture) {
		printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}
	if (!SDL_SetTexturePalette(viM->dynamic_texture, viM->sdl_palette)) {
		printf("SDL_SetTexturePalette Error: %s\n", SDL_GetError());
		return true;
	}
	if (!SDL_SetTextureScaleMode(
		    viM->dynamic_texture, SDL_SCALEMODE_NEAREST)) {
		printf("Warning: failed to set nearest neighbor scaling\n");
	}
	viM->current_res_w = 0;
	viM->current_res_h = 0;

	SDL_StartTextInput(viM->window);
	return false;
}