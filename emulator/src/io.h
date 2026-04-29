#pragma once
#include "vm.h"

void interrupt_pushtostack(struct VirtualMachine *viM);
void interrupt_timer(struct VirtualMachine *viM, const unsigned int *timer);
void interrupt_input(struct VirtualMachine *viM);
