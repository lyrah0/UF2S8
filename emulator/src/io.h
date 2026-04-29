#pragma once
#include "vm.h"

void interrupt_pushtostack(struct VirtualMachine *viM);
void interrupt_timer(struct VirtualMachine *viM, uint64_t ticks_ns);
void interrupt_input(struct VirtualMachine *viM);
