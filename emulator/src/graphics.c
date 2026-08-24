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

static void update_colors(
	struct VirtualMachine *viM, SDL_Color *colors, uint8_t mode)
{
	if (mode == 0) { // 8bpp RGB332 direct mode
		static SDL_Color s_rgb332[256];
		static bool s_rgb332_init = false;
		if (!s_rgb332_init) {
			for (int i = 0; i < 256; i++) {
				s_rgb332[i].r = (uint8_t)((i >> 5) * 255 / 7);
				s_rgb332[i].g =
					(uint8_t)(((i >> 2) & 0x07) * 255 / 7);
				s_rgb332[i].b =
					(uint8_t)((i & 0x03) * 255 / 3);
				s_rgb332[i].a = 255;
			}
			s_rgb332_init = true;
		}
		SDL_SetPaletteColors(viM->sdl_palette, s_rgb332, 0, 256);
	} else { // Indexed color modes (< 8bpp): read 16-entry palette from VRAM 0xFFF0..0xFFFF
		for (int i = 0; i < 16; i++) {
			uint8_t pal = viM->vram[0xFFF0 + i];
			colors[i].r = (uint8_t)((pal >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((pal >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((pal & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, 16);
	}
}

void render_graphics_frame(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }

	uint8_t mode = viM->hw_regs[HW_HW_CTRL - 0xFFF0] & 0x03;
	const uint8_t *source_data = viM->vram;
	const int res_w = 256;
	const int res_h = 256;

	if (viM->current_res_w != res_w || viM->current_res_h != res_h) {
		SDL_SetWindowSize(viM->window, 512, 512);
		SDL_SetRenderLogicalPresentation(viM->renderer, res_w, res_h,
			SDL_LOGICAL_PRESENTATION_LETTERBOX);
		viM->current_res_w = res_w;
		viM->current_res_h = res_h;
	}

	SDL_Color colors[256];
	update_colors(viM, colors, mode);

	int pixels = res_w * res_h; // 65536
	int vram_bytes = 0;
	switch (mode) {
	case 0:
		vram_bytes = pixels; // 64KiB
		break;
	case 1:
		vram_bytes = pixels >> 1; // 32KiB
		break;
	case 2:
		vram_bytes = pixels >> 2; // 16KiB
		break;
	case 3:
		vram_bytes = pixels >> 3; // 8KiB
		break;
	default:
		break;
	}

	if (mode == 1) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			viM->processed_vram[(ptrdiff_t)(i * 2)] =
				(vbyte >> 4) & 0x0F;
			viM->processed_vram[(ptrdiff_t)(i * 2) + 1] = vbyte &
				0x0F;
		}
		source_data = viM->processed_vram;
	} else if (mode == 2) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			for (int j = 0; j < 4; j++) {
				viM->processed_vram[(i * 4) + j] =
					(vbyte >> ((3 - j) * 2)) & 0x03;
			}
		}
		source_data = viM->processed_vram;
	} else if (mode == 3) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			for (int j = 0; j < 8; j++) {
				viM->processed_vram[(i * 8) + j] =
					(vbyte >> (7 - j)) & 0x01;
			}
		}
		source_data = viM->processed_vram;
	}

	SDL_UpdateTexture(viM->dynamic_texture, nullptr, source_data, res_w);
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