#include "graphics.h"
#include "memory.h"
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
#include <stdlib.h>

static const int MODE_RES[14] = { GFX_MODE0_RES, GFX_MODE1_RES, GFX_MODE2_RES,
	GFX_MODE3_RES, GFX_MODE4_RES, GFX_MODE5_RES, GFX_MODE6_RES,
	GFX_MODE7_RES, GFX_MODE8_RES, GFX_MODE9_RES, GFX_MODE10_RES,
	GFX_MODE11_RES, GFX_MODE12_RES, GFX_MODE13_RES };

static const int MODE_VRAM_BYTES[14] = { (int)(GFX_MODE0_RES * 1.6) *
		GFX_MODE0_RES,
	(int)(GFX_MODE1_RES * 1.6) * GFX_MODE1_RES,
	(int)(GFX_MODE2_RES * 1.6) * GFX_MODE2_RES,
	(int)(GFX_MODE3_RES * 1.6) * GFX_MODE3_RES,
	(int)(GFX_MODE4_RES * 1.6) * GFX_MODE4_RES / 2,
	(int)(GFX_MODE5_RES * 1.6) * GFX_MODE5_RES / 2,
	(int)(GFX_MODE6_RES * 1.6) * GFX_MODE6_RES / 2,
	(int)(GFX_MODE7_RES * 1.6) * GFX_MODE7_RES / 4,
	(int)(GFX_MODE8_RES * 1.6) * GFX_MODE8_RES / 4,
	(int)(GFX_MODE9_RES * 1.6) * GFX_MODE9_RES / 4,
	(int)(GFX_MODE10_RES * 1.6) * GFX_MODE10_RES / 8,
	(int)(GFX_MODE11_RES * 1.6) * GFX_MODE11_RES / 8,
	(int)(GFX_MODE12_RES * 1.6) * GFX_MODE12_RES / 8,
	(int)(GFX_MODE13_RES * 1.6) * GFX_MODE13_RES / 8 };

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

static uint8_t read_pixel_vram(struct VirtualMachine *viM, uint8_t mode,
	int pos_x, int pos_y, int stride)
{
	if (mode <= 3) { // 8bpp
		return viM->vram[(pos_y * stride) + pos_x];
	}
	if (mode <= 6) { // 4bpp
		uint8_t val = viM->vram[(pos_y * (stride / 2)) + (pos_x / 2)];
		return (pos_x % 2 == 0) ? (val >> 4) : (val & 0x0F);
	}
	if (mode <= 9) { // 2bpp
		uint8_t val = viM->vram[(pos_y * (stride / 4)) + (pos_x / 4)];
		return (val >> ((3 - (pos_x % 4)) * 2)) & 0x03;
	}
	if (mode <= 13) { // 1bpp
		uint8_t val = viM->vram[(pos_y * (stride / 8)) + (pos_x / 8)];
		return (val >> (7 - (pos_x % 8))) & 0x01;
	}
	return 0;
}

static void write_pixel_vram(struct VirtualMachine *viM, uint8_t mode,
	int pos_x, int pos_y, int stride, uint8_t color, uint8_t flags,
	uint8_t alpha, uint8_t cmd)
{
	uint8_t pixel_to_write = color;

	if (mode <= 3) { // 8bpp
		uint16_t vram_addr = (pos_y * stride) + pos_x;
		if (cmd == 0x06) { // XOR
			pixel_to_write = viM->vram[vram_addr] ^ color;
		} else if (flags & 0x04) { // Alpha
			uint8_t dst_color = viM->vram[vram_addr];
			int src_r = (pixel_to_write >> 5) & 0x07;
			int src_g = (pixel_to_write >> 2) & 0x07;
			int src_b = pixel_to_write & 0x03;
			int dst_r = (dst_color >> 5) & 0x07;
			int dst_g = (dst_color >> 2) & 0x07;
			int dst_b = dst_color & 0x03;
			int res_r =
				((src_r * alpha) + (dst_r * (255 - alpha))) /
				255;
			int res_g =
				((src_g * alpha) + (dst_g * (255 - alpha))) /
				255;
			int res_b =
				((src_b * alpha) + (dst_b * (255 - alpha))) /
				255;
			pixel_to_write =
				(uint8_t)((res_r << 5) | (res_g << 2) | res_b);
		}
		viM->vram[vram_addr] = pixel_to_write;
	} else if (mode <= 6) { // 4bpp
		uint16_t vram_addr = (pos_y * (stride / 2)) + (pos_x / 2);
		uint8_t val = viM->vram[vram_addr];
		if (cmd == 0x06) {
			pixel_to_write = ((pos_x % 2 == 0) ? (val >> 4) :
							     (val & 0x0F)) ^
				(color & 0x0F);
		}
		if (pos_x % 2 == 0) {
			val = (uint8_t)((val & 0x0F) | (pixel_to_write << 4));
		} else {
			val = (uint8_t)((val & 0xF0) |
				(pixel_to_write & 0x0F));
		}
		viM->vram[vram_addr] = val;
	} else if (mode <= 9) { // 2bpp
		uint16_t vram_addr = (pos_y * (stride / 4)) + (pos_x / 4);
		uint8_t val = viM->vram[vram_addr];
		int shift = (3 - (pos_x % 4)) * 2;
		if (cmd == 0x06) {
			pixel_to_write = ((val >> shift) & 0x03) ^
				(color & 0x03);
		}
		val = (uint8_t)((val & ~(0x03 << shift)) |
			((pixel_to_write & 0x03) << shift));
		viM->vram[vram_addr] = val;
	} else if (mode <= 13) { // 1bpp
		uint16_t vram_addr = (pos_y * (stride / 8)) + (pos_x / 8);
		uint8_t val = viM->vram[vram_addr];
		int shift = (7 - (pos_x % 8));
		if (cmd == 0x06) {
			pixel_to_write = ((val >> shift) & 0x01) ^
				(color & 0x01);
		}
		val = (uint8_t)((val & ~(0x01 << shift)) |
			((pixel_to_write & 0x01) << shift));
		viM->vram[vram_addr] = val;
	}
}

static int get_palette_size(uint8_t mode)
{
	switch (mode) {
	case 4:
	case 5:
	case 6:
		return 16;
	case 7:
	case 8:
	case 9:
		return 4;
	case 10:
	case 11:
	case 12:
	case 13:
		return 2;
	default:
		return 16;
	}
}

static void update_colors(
	struct VirtualMachine *viM, SDL_Color *colors, uint8_t mode)
{
	if (mode > 3) {
		int pal_size = get_palette_size(mode);
		for (int i = 0; i < pal_size; i++) {
			uint8_t val = viM->vram[HW_PALETTE_START + i];
			colors[i].r = (uint8_t)((val >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((val >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((val & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, pal_size);
	} else {
		for (int i = 0; i < 256; i++) {
			colors[i].r = (uint8_t)((i >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((i >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((i & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, 256);
	}
}

void render_graphics_frame(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }

	uint8_t mode = memory_read(viM, HW_GFX_CTRL) & 0x0F;
	if (mode > 13) { mode = 0; }
	uint8_t *source_data = viM->vram;
	int res = MODE_RES[mode];
	SDL_Color colors[256];

	update_colors(viM, colors, mode);

	int vram_bytes = MODE_VRAM_BYTES[mode];
	if (mode >= 4 && mode <= 6) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			viM->processed_vram[(ptrdiff_t)(i * 2)] =
				(vbyte >> 4) & 0x0F;
			viM->processed_vram[(ptrdiff_t)(i * 2) + 1] = vbyte &
				0x0F;
		}
		source_data = viM->processed_vram;
	} else if (mode >= 7 && mode <= 9) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			for (int j = 0; j < 4; j++) {
				viM->processed_vram[(i * 4) + j] =
					(vbyte >> ((3 - j) * 2)) & 0x03;
			}
		}
		source_data = viM->processed_vram;
	} else if (mode >= 10 && mode <= 13) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			for (int j = 0; j < 8; j++) {
				viM->processed_vram[(i * 8) + j] =
					(vbyte >> (7 - j)) & 0x01;
			}
		}
		source_data = viM->processed_vram;
	}

	SDL_UpdateTexture(
		viM->textures[mode], nullptr, source_data, (int)(res * 1.6));
	SDL_RenderClear(viM->renderer);
	SDL_RenderTexture(
		viM->renderer, viM->textures[mode], nullptr, nullptr);
	SDL_RenderPresent(viM->renderer);
}

static void blit_line(struct VirtualMachine *viM, uint8_t mode, uint16_t src_x,
	uint16_t src_y, int16_t dst_x, int16_t dst_y, uint16_t dst_stride,
	uint8_t color, uint8_t flags, uint8_t alpha, uint8_t cmd,
	int clip_x_min, int clip_x_max, int clip_y_min, int clip_y_max)
{
	int start_x = src_x;
	int start_y = src_y;
	int end_x = dst_x;
	int end_y = dst_y;
	int delta_x = abs(end_x - start_x);
	int delta_y = -abs(end_y - start_y);
	int step_x = start_x < end_x ? 1 : -1;
	int step_y = start_y < end_y ? 1 : -1;
	int err = delta_x + delta_y;

	while (1) {
		if (start_x >= clip_x_min && start_x <= clip_x_max &&
			start_y >= clip_y_min && start_y <= clip_y_max) {
			write_pixel_vram(viM, mode, start_x, start_y,
				dst_stride, color, flags, alpha, cmd);
		}
		if (start_x == end_x && start_y == end_y) { break; }
		int err_twice = 2 * err;
		if (err_twice >= delta_y) {
			err += delta_y;
			start_x += step_x;
		}
		if (err_twice <= delta_x) {
			err += delta_x;
			start_y += step_y;
		}
	}
}

static uint8_t get_source_pixel(struct VirtualMachine *viM, uint8_t mode,
	uint16_t src_x, uint16_t src_y, uint16_t src_stride, int cur_x,
	int cur_y, uint8_t cmd, int res_w, int res_h)
{
	if (cmd <= 0x03) { // From Memory
		uint16_t src_addr = src_x + (cur_y * src_stride) + cur_x;
		return memory_read(viM, src_addr);
	}
	// From VRAM
	int in_x = src_x + cur_x;
	int in_y = src_y + cur_y;
	if (in_x < 0 || in_x >= res_w || in_y < 0 || in_y >= res_h) {
		return 0;
	}
	return read_pixel_vram(viM, mode, in_x, in_y, src_stride);
}

static void blit_rectangle(struct VirtualMachine *viM, uint8_t mode,
	uint16_t src_x, uint16_t src_y, int16_t dst_x, int16_t dst_y,
	uint16_t width, uint16_t height, uint16_t src_stride,
	uint16_t dst_stride, uint8_t color, uint8_t flags, uint8_t alpha,
	uint8_t cmd, int clip_x_min, int clip_x_max, int clip_y_min,
	int clip_y_max, int res_w, int res_h)
{
	for (int cur_y = 0; cur_y < height; cur_y++) {
		for (int cur_x = 0; cur_x < width; cur_x++) {
			int out_x = dst_x +
				((flags & 0x01) ? (width - 1 - cur_x) : cur_x);
			int out_y = dst_y +
				((flags & 0x02) ? (height - 1 - cur_y) :
						  cur_y);

			if (out_x < clip_x_min || out_x > clip_x_max ||
				out_y < clip_y_min || out_y > clip_y_max) {
				continue;
			}

			uint8_t pixel_to_write = color;

			if (cmd >= 0x02 && cmd <= 0x05) { // Blit commands
				uint8_t src_pixel = get_source_pixel(viM, mode,
					src_x, src_y, src_stride, cur_x, cur_y,
					cmd, res_w, res_h);

				if ((cmd == 0x03 || cmd == 0x05) &&
					src_pixel == color) {
					continue; // Transparent
				}
				pixel_to_write = src_pixel;
			}

			write_pixel_vram(viM, mode, out_x, out_y, dst_stride,
				pixel_to_write, flags, alpha, cmd);
		}
	}
}

void execute_blit(struct VirtualMachine *viM, uint8_t cmd)
{
	if (cmd == 0) { return; }

	// Read registers
	uint16_t src_x = memory_read(viM, HW_BLIT_SRC_X_L) |
		(memory_read(viM, HW_BLIT_SRC_X_H) << 8);
	uint16_t src_y = memory_read(viM, HW_BLIT_SRC_Y_L) |
		(memory_read(viM, HW_BLIT_SRC_Y_H) << 8);
	int16_t dst_x = (int16_t)(memory_read(viM, HW_BLIT_DST_X_L) |
		(memory_read(viM, HW_BLIT_DST_X_H) << 8));
	int16_t dst_y = (int16_t)(memory_read(viM, HW_BLIT_DST_Y_L) |
		(memory_read(viM, HW_BLIT_DST_Y_H) << 8));

	uint16_t width = memory_read(viM, HW_BLIT_WIDTH_L) |
		(memory_read(viM, HW_BLIT_WIDTH_H) << 8);
	uint16_t height = memory_read(viM, HW_BLIT_HEIGHT_L) |
		(memory_read(viM, HW_BLIT_HEIGHT_H) << 8);
	uint16_t src_stride = memory_read(viM, HW_BLIT_SRC_STRIDE_L) |
		(memory_read(viM, HW_BLIT_SRC_STRIDE_H) << 8);
	uint16_t dst_stride = memory_read(viM, HW_BLIT_DST_STRIDE_L) |
		(memory_read(viM, HW_BLIT_DST_STRIDE_H) << 8);

	uint8_t color = memory_read(viM, HW_BLIT_COLOR);
	uint8_t alpha = memory_read(viM, HW_BLIT_ALPHA);
	uint8_t flags = memory_read(viM, HW_BLIT_FLAGS);

	uint8_t mode = memory_read(viM, HW_GFX_CTRL) & 0x0F;
	if (mode > 13) { mode = 0; }
	int res = MODE_RES[mode];
	int res_w = (int)(res * 1.6);
	int res_h = res;

	// Use res_w as default stride if 0
	if (!src_stride) { src_stride = (cmd <= 0x03) ? width : res_w; }
	if (!dst_stride) { dst_stride = res_w; }

	// Clipping bounds
	int clip_x_min = (flags & 0x08) ?
		(memory_read(viM, HW_BLIT_CLIP_X_MIN_L) |
			(memory_read(viM, HW_BLIT_CLIP_X_MIN_H) << 8)) :
		0;
	int clip_x_max = (flags & 0x08) ?
		(memory_read(viM, HW_BLIT_CLIP_X_MAX_L) |
			(memory_read(viM, HW_BLIT_CLIP_X_MAX_H) << 8)) :
		res_w - 1;
	int clip_y_min = (flags & 0x08) ?
		(memory_read(viM, HW_BLIT_CLIP_Y_MIN_L) |
			(memory_read(viM, HW_BLIT_CLIP_Y_MIN_H) << 8)) :
		0;
	int clip_y_max = (flags & 0x08) ?
		(memory_read(viM, HW_BLIT_CLIP_Y_MAX_L) |
			(memory_read(viM, HW_BLIT_CLIP_Y_MAX_H) << 8)) :
		res_h - 1;

	if (cmd == 0x07) { // LINE DRAW
		blit_line(viM, mode, src_x, src_y, dst_x, dst_y, dst_stride,
			color, flags, alpha, cmd, clip_x_min, clip_x_max,
			clip_y_min, clip_y_max);
	} else {
		blit_rectangle(viM, mode, src_x, src_y, dst_x, dst_y, width,
			height, src_stride, dst_stride, color, flags, alpha,
			cmd, clip_x_min, clip_x_max, clip_y_min, clip_y_max,
			res_w, res_h);
	}
}

bool init_sdl(struct VirtualMachine *viM) // NOLINT
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init Error: %s\n", SDL_GetError());
		return true;
	}

	viM->window = SDL_CreateWindow("UF2S8 Emulator",
		(int)(DEFAULT_DISPLAY_HEIGHT * 1.6), DEFAULT_DISPLAY_HEIGHT,
		0);
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

	for (int i = 0; i < 14; i++) {
		int res = MODE_RES[i];
		int tex_width = (int)(res * 1.6);
		int tex_height = res;
		viM->textures[i] = SDL_CreateTexture(viM->renderer,
			SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STREAMING,
			tex_width, tex_height);
		if (!viM->textures[i]) {
			printf("SDL_CreateTexture Error (mode %d): %s\n", i,
				SDL_GetError());
			SDL_DestroyRenderer(viM->renderer);
			SDL_DestroyWindow(viM->window);
			SDL_Quit();
			return true;
		}
		if (!SDL_SetTexturePalette(
			    viM->textures[i], viM->sdl_palette)) {
			printf("SDL_SetTexturePalette Error (mode %d): %s\n",
				i, SDL_GetError());
			return true;
		}
		if (!SDL_SetTextureScaleMode(
			    viM->textures[i], SDL_SCALEMODE_NEAREST)) {
			printf("Warning: failed to set nearest neighbor scaling for mode %d\n",
				i);
		}
	}

	SDL_StartTextInput(viM->window);
	return false;
}