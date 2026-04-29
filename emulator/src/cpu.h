#pragma once
#include "vm.h"

bool decode_execute(struct VirtualMachine *viM, uint16_t instruction);
void print_state(const struct VirtualMachine *viM, uint16_t instruction);
