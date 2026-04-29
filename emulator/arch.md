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
|BLIT_SRC_X_L    |0xFEC0|Blitter source x coordinate / address low
|BLIT_SRC_X_H    |0xFEC1|Blitter source x coordinate / address high
|BLIT_SRC_Y_L    |0xFEC2|Blitter source y coordinate low
|BLIT_SRC_Y_H    |0xFEC3|Blitter source y coordinate high
|BLIT_DST_X_L    |0xFEC4|Blitter destination x coordinate low
|BLIT_DST_X_H    |0xFEC5|Blitter destination x coordinate high
|BLIT_DST_Y_L    |0xFEC6|Blitter destination y coordinate low
|BLIT_DST_Y_H    |0xFEC7|Blitter destination y coordinate high
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
|BLIT_WIDTH_L      |0xFED4|Blitter width low
|BLIT_WIDTH_H      |0xFED5|Blitter width high
|BLIT_HEIGHT_L     |0xFED6|Blitter height low
|BLIT_HEIGHT_H     |0xFED7|Blitter height high
|BLIT_ALPHA      |0xFEDC|Blitter alpha level (0=transparent, 255=opaque)
|BLIT_FLAGS      |0xFEDD|Blitter flags
|BLIT_COLOR      |0xFEDE|Blitter color
|BLIT_CMD        |0xFEDF|Blitter command
|GFX_ADDR_L      |0xFEE0|Graphics VRAM address low
|GFX_ADDR_H      |0xFEE1|Graphics VRAM address high
|GFX_DATA        |0xFEEE|Graphics VRAM data
|GFX_CTRL        |0xFEEF|graphics control
|terminal_out    |0xFEF0|terminal output
|keyboard_data   |0xFEF1|keyboard data
|keyboard_status |0xFEF2|keyboard status
|timer_mult      |0xFEFE|timer multiplier in hertz (1/hertz) 0=disabled
|hardware_control|0xFEFF|hardware control

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
|7-6	|Reserved|
|5      |auto increment address on read (VRAM)
|4      |auto increment address on write (VRAM)
|3-0	|graphics mode

* graphics modes:
        - 0-3 8bpp:
                - 0 = 320x200 (64000/0xFA00 bytes)
                - 1 = 272x180 (48960/0xBF00 bytes)
                - 2 = 256x160 (40960/0xA000 bytes)
                - 3 = 224x140 (31360/0x7A60 bytes)
        - 4-6 4bpp:
                - 4 = 448x280 (62720/0xF500 bytes)
                - 5 = 400x240 (48000/0xBB80 bytes)
                - 6 = 320x200 (32000/0x7D00 bytes)
        - 7-9 2bpp:
                - 7 = 640x400 (64000/0xFA00 bytes)
                - 8 = 448x280 (31360/0x7A60 bytes)
                - 9 = 320x200 (16000/0x3E80 bytes)
        - 10-13 1bpp:
                - 10 = 912x570 (64980/0xFDD4 bytes)
                - 11 = 640x400 (32000/0x7D00 bytes)
                - 12 = 448x280 (15680/0x3D40 bytes)
                - 13 = 320x200 (8000/0x1F40 bytes)
        - 14-15 - Reserved


hardware control format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1	|keyboard interrupt enable|
|0	|Timer enable|


keyboard_status format:

|Bit	|Description|
|-------|-----------|
|7-2	|Reserved|
|1	|Press-Release (0=Press, 1=Release)|
|0	|Keyboard ready (0=Not ready, 1=Ready)|

graphics VRAM:

|Address|size|purpose|
|-------|----|-------|
|0xFFF0|16B|palette