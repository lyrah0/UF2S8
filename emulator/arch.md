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

|Name	          |Address|Description|
|-----------------|------|-----------|
|reserved         |0xFE00-0xFEBF|Reserved
|BLIT_SRC_X_L     |0xFEC0|Blitter source x coordinate / address low
|BLIT_SRC_X_H     |0xFEC1|Blitter source x coordinate / address high
|BLIT_SRC_Y_L     |0xFEC2|Blitter source y coordinate low
|BLIT_SRC_Y_H     |0xFEC3|Blitter source y coordinate high
|BLIT_DST_X_L     |0xFEC4|Blitter destination x coordinate low
|BLIT_DST_X_H     |0xFEC5|Blitter destination x coordinate high
|BLIT_DST_Y_L     |0xFEC6|Blitter destination y coordinate low
|BLIT_DST_Y_H     |0xFEC7|Blitter destination y coordinate high
|BLIT_CLIP_X_MIN_L|0xFEC8|Blitter clip x coordinate minimum low
|BLIT_CLIP_X_MIN_H|0xFEC9|Blitter clip x coordinate minimum high
|BLIT_CLIP_Y_MIN_L|0xFECA|Blitter clip y coordinate minimum low
|BLIT_CLIP_Y_MIN_H|0xFECB|Blitter clip y coordinate minimum high
|BLIT_CLIP_X_MAX_L|0xFECC|Blitter clip x coordinate maximum low
|BLIT_CLIP_X_MAX_H|0xFECD|Blitter clip x coordinate maximum high
|BLIT_CLIP_Y_MAX_L|0xFECE|Blitter clip y coordinate maximum low
|BLIT_CLIP_Y_MAX_H|0xFECF|Blitter clip y coordinate maximum high
|BLIT_SRC_STRIDE_L|0xFED0|Blitter source stride low
|BLIT_SRC_STRIDE_H|0xFED1|Blitter source stride high
|BLIT_DST_STRIDE_L|0xFED2|Blitter destination stride low
|BLIT_DST_STRIDE_H|0xFED3|Blitter destination stride high
|BLIT_WIDTH_L     |0xFED4|Blitter width low
|BLIT_WIDTH_H     |0xFED5|Blitter width high
|BLIT_HEIGHT_L    |0xFED6|Blitter height low
|BLIT_HEIGHT_H    |0xFED7|Blitter height high
|reserved         |0xFED8-0xFEDB|Reserved
|BLIT_ALPHA       |0xFEDC|Blitter alpha level (0=transparent, 255=opaque)
|BLIT_FLAGS       |0xFEDD|Blitter flags
|BLIT_COLOR       |0xFEDE|Blitter color
|BLIT_CMD         |0xFEDF|Blitter command
|GFX_ADDR_L       |0xFEE0|Graphics VRAM address low
|GFX_ADDR_H       |0xFEE1|Graphics VRAM address high
|GFX_WIDTH        |0xFEE2|Graphics width (Pixels = Value * 8)
|GFX_HEIGHT       |0xFEE3|Graphics height (Pixels = Value * 8)
|reserved         |0xFEE4-0xFEED|Reserved
|GFX_DATA         |0xFEEE|Graphics VRAM data
|GFX_CTRL         |0xFEEF|graphics control
|TERM_OUT         |0xFEF0|terminal output
|KBD_DATA         |0xFEF1|keyboard data
|KBD_STATUS       |0xFEF2|keyboard status
|reserved         |0xFEF3-0xFEFC|Reserved
|BANK_SEL         |0xFEFD|Bank select register (Bits 0-3: Window 0, Bits 4-7: Window 1)
|TIMER_HZ         |0xFEFE|timer multiplier in hertz (1/hertz) 0=disabled
|HW_CTRL          |0xFEFF|hardware control

Memory Map:

|Address Range|Size|Type|Description|
|-------------|----|----|-----------|
|0x0000-0x7FFF|32KB|Banked|Window 0 (Mapped from 512KB pool, 16 banks)|
|0x8000-0xBFFF|16KB|Banked|Window 1 (Mapped from 128KB pool, 8 banks)|
|0xC000-0xDFFF|8KB|Banked|Window 2 (Mapped from 16KB pool, 2 banks)|
|0xE000-0xFE00|7.5KB|Fixed|Fixed RAM (Stack, Heap, etc.)|
|0xFE00-0xFEFF|256B|Fixed|Hardware Registers|
|0xFF00-0xFFFF|256B|Fixed|Vector table|

BANK_SEL format:

|Bit	|Description|
|-------|-----------|
|7	|Window 2 bank
|6-4	|Window 1 bank
|3-0	|Window 0 bank

BLIT_FLAGS format:

|Bit	|Description|
|-------|-----------|
|7-4	|Reserved|
|3      |Clip enable
|2	|Alpha blending enable
|1	|Vertical flip
|0	|Horizontal flip

BLIT_CMD list:

|Value |Name               |Description|
|------|-------------------|-----------|
|0x01  |FILL_RECT          |Fill destination rectangle with BLIT_COLOR
|0x02  |BLIT_MEM           |Copy block from Memory to VRAM
|0x03  |BLIT_MEM_TRANS     |Copy block from Memory to VRAM (skip BLIT_COLOR pixels)
|0x04  |BLIT_VRAM          |Copy block from VRAM to VRAM
|0x05  |BLIT_VRAM_TRANS    |Copy block from VRAM to VRAM (skip BLIT_COLOR pixels)
|0x06  |BLIT_XOR           |XOR source pixels with destination
|0x07  |LINE               |Draw line from (SRC_X, SRC_Y) to (DST_X, DST_Y) using BLIT_COLOR


GFX_CTRL format:

|Bit	|Description|
|-------|-----------|
|7-4	|Reserved|
|3      |auto increment address on read (VRAM)
|2      |auto increment address on write (VRAM)
|1-0	|color depth mode

* color depth modes:
        - 0 = 8bpp(RGB332)
        - 1 = 4bpp
        - 2 = 2bpp
        - 3 = 1bpp


HW_CTRL format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1	|keyboard interrupt enable|
|0	|Timer enable|


KBD_STATUS format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1	|Press-Release (0=Press, 1=Release)|
|0	|Keyboard ready (0=Not ready, 1=Ready)|

graphics VRAM:

|Address|size|purpose|
|-------|----|-------|
|0xFFF0|16B|palette