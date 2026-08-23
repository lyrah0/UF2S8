#pragma once
#include "vm.h"

void memory_init(struct VirtualMachine *viM);
void memory_set_bank(struct VirtualMachine *viM, uint8_t win, uint8_t bank);
uint16_t fetch_instruction(struct VirtualMachine *viM);
void memory_dump(struct VirtualMachine *viM);
void memory_write(struct VirtualMachine *viM, uint16_t address, uint8_t value);
uint8_t memory_read(struct VirtualMachine *viM, uint16_t address);