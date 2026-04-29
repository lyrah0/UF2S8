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
|palette         |0xFEE0|16 bytes of RGB332 palette entries (modes 0-2)
|terminal_out    |0xFEF0|terminal output
|keyboard_in     |0xFEF1|keyboard input
|graphics_mode   |0xFEFD|graphics control
|timer_mult      |0xFEFE|timer multiplier
|hardware_control|0xFEFF|hardware control

hardware control format:

|Bit	|Description|
|-------|-----------|
|7-1	|Reserved|
|0	|Timer enable|


graphics_control format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1-0	|Graphics mode (0=1bpp, 1=2bpp, 2=4bpp, 3=8bpp (RGB332))|
