#pragma once
#include "vm.h"

int get_keypress_nonblocking();
void interrupt_pushtostack(struct VirtualMachine *viM);
void interrupt_timer(struct VirtualMachine *viM, const unsigned int *timer);
void interrupt_input(struct VirtualMachine *viM, const unsigned int *timer);
