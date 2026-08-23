#pragma once
#include "vm.h"
#include <stdint.h>

void trigger_interrupt(struct VirtualMachine *viM, uint8_t int_id);
void check_and_dispatch_interrupts(
	struct VirtualMachine *viM, uint64_t ticks_ns);
void handle_uart_events(struct VirtualMachine *viM);
