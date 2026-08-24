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
|0x12|UART      |UART RX interrupt
|0x13|Vsync	|Vertical sync interrupt
|0x14-0x1F|Reserved|Reserved

hardware registers:

|Name		|Address|Description|
|---------------|------|-----------|
|TIMER_HZ	|0xFFF0|timer multiplier in hertz (1/hertz) 0=disabled
|HW_CTRL	|0xFFF1|hardware control
|HW_STATUS	|0xFFF2|keyboard status
|KBD_DATA	|0xFFF3|keyboard data
|UART_STATUS	|0xFFF4|UART status register (R)
|UART_DATA	|0xFFF5|UART data register (R/W)
|reserved 	|0xFFF6-0xFFF7|Reserved
|BANK_SEL_0	|0xFFF8|Window 0 bank select register (8-bit)
|BANK_SEL_1	|0xFFF9|Window 1 bank select register (8-bit)
|BANK_SEL_2	|0xFFFA|Window 2 bank select register (8-bit)
|BANK_SEL_3	|0xFFFB|Window 3 bank select register (8-bit)
|BANK_SEL_4	|0xFFFC|Window 4 bank select register (8-bit)
|BANK_SEL_5	|0xFFFD|Window 5 bank select register (8-bit)
|BANK_SEL_6	|0xFFFE|Window 6 bank select register (8-bit)
|BANK_SEL_7	|0xFFFF|Window 7 bank select register (8-bit)

Memory Map:

|Address Range|Size|Type|Description|
|-------------|----|----|-----------|
|0x0000-0x1FFF|8KiB|Banked|Window 0 (Mapped to 256 banks)|
|0x2000-0x3FFF|8KiB|Banked|Window 1 (Mapped to 256 banks)|
|0x4000-0x5FFF|8KiB|Banked|Window 2 (Mapped to 256 banks)|
|0x6000-0x7FFF|8KiB|Banked|Window 3 (Mapped to 256 banks)|
|0x8000-0x9FFF|8KiB|Banked|Window 4 (Mapped to 256 banks)|
|0xA000-0xBFFF|8KiB|Banked|Window 5 (Mapped to 256 banks)|
|0xC000-0xDFFF|8KiB|Banked|Window 6 (Mapped to 256 banks)|
|0xE000-0xFFEF|7.98KiB|Banked|Window 7 (Mapped to 256 banks)|
|0xFFF0-0xFFFF|16B|Fixed|Hardware Registers|

Banking format:

256 banks of 8KB (2MB total addressable pool):
- Banks 0–127 (0x00–0x7F): 128 ROM banks (1024KB, Read-only)
- Banks 128–247 (0x80–0xF7): 120 RAM banks (960KB, Read/Write)
- Banks 248–255 (0xF8–0xFF): 8 VRAM banks (64KB, Read/Write)

HW_CTRL format:

|Bit	|Description|
|-------|-----------|
|7	|UART receiver interrupt enable|
|6	|Keyboard interrupt enable|
|5	|Vsync interrupt enable|
|4-3	|Reserved|
|1-0	|Color depth mode|

* color depth modes:
	- all modes are 256x256 resolution
	- 0 = 8bpp(RGB332) - 64KiB
	- 1 = 4bpp - 32KiB
	- 2 = 2bpp - 16KiB
	- 3 = 1bpp - 8KiB

HW_STATUS format:

|Bit	|Description|
|-------|-----------|
|7-4	|Reserved|
|3	|UART transmitter empty (Always 1 in emulator)|
|2	|UART receiver ready (0=No data, 1=Data available)|
|1	|Keyboard Press-Release (0=Press, 1=Release)|
|0	|Keyboard ready (0=Not ready, 1=Ready)|

graphics VRAM:

|Address|size|purpose|
|-------|----|-------|
|0xFFF0|16B|palette(only valid in less than 8bpp)|