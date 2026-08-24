#pragma once
#include "vm.h"

void handle_graphics_events(struct VirtualMachine *viM);
void render_graphics_frame(struct VirtualMachine *viM);
bool init_sdl(struct VirtualMachine *viM);