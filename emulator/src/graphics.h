#pragma once
#include "vm.h"

void handle_graphics_events(struct VirtualMachine *viM);
void render_graphics_frame(struct VirtualMachine *viM);
void render_scanline(struct VirtualMachine *viM, int y);
__attribute__((pure)) int get_graphics_mode_height(
	const struct VirtualMachine *viM);
bool init_sdl(struct VirtualMachine *viM);