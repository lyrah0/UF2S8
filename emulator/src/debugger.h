#pragma once
#include "vm.h"

void disassemble(uint16_t instruction);
void debug_prompt(struct VirtualMachine *viM, uint16_t instruction);