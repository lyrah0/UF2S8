Architectural info of the emulator:

interrupts:

|ID  |Name	|Description|
|----|----------|-----------|
|0x00|Halt	|Halt execution
|0x01|Breakpoint|Breakpoint
|0x02|Illegal instruction|Illegal instruction
|0x03-0x0F|Reserved|Reserved
|0x10|Timer	|Timer interrupt
|0x11|Keyboard  |Keyboard interrupt
|0x12-0x1F|Reserved|Reserved

hardware registers:

|Name	         |Address|Description|
|----------------|------|-----------|
|palette         |0xFEE0|16 bytes of RGB332 palette entries (modes 0-2)
|terminal_out    |0xFEF0|terminal output
|keyboard_data   |0xFEF1|keyboard data
|keyboard_status |0xFEF2|keyboard status
|graphics_control|0xFEFD|graphics control
|timer_mult      |0xFEFE|timer multiplier in hertz (1/hertz) 0=disabled
|hardware_control|0xFEFF|hardware control

hardware control format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1	|keyboard interrupt enable|
|0	|Timer enable|


graphics_control format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1-0	|Graphics mode

* graphics modes:
        - 0 = 1bpp 256x256
        - 1 = 2bpp 180x180
        - 2 = 4bpp 128x128
        - 3 = 8bpp (RGB332) 90x90

keyboard_status format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1	|Press-Release (0=Press, 1=Release)|
|0	|Keyboard ready (0=Not ready, 1=Ready)|
