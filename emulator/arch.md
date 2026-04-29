Architectural info of the emulator:

interrupts:

|ID  |Name	|Description|
|----|----------|-----------|
|0x00|Halt	|Halt execution
|0x01|Breakpoint|Breakpoint
|0x02|Timer	|Timer interrupt
|0x03|Keyboard  |Keyboard interrupt

hardware registers:

|Name	         |Address|Description|
|----------------|------|-----------|
|terminal_out    |0xFEF0|terminal output
|keyboard_in     |0xFEF1|keyboard input
|timer_mult      |0xFEFE|timer multiplier
|hardware_control|0xFEFF|hardware control

hardware control format:

|Bit	|Description|
|-------|-----------|
|7-1	|Reserved|
|0	|Timer enable|
