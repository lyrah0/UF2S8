; Hardware Registers
.equ HW_BLIT_SRC_X_L,      0xFEC0
.equ HW_BLIT_SRC_X_H,      0xFEC1
.equ HW_BLIT_SRC_Y_L,      0xFEC2
.equ HW_BLIT_SRC_Y_H,      0xFEC3
.equ HW_BLIT_DST_X_L,      0xFEC4
.equ HW_BLIT_DST_X_H,      0xFEC5
.equ HW_BLIT_DST_Y_L,      0xFEC6
.equ HW_BLIT_DST_Y_H,      0xFEC7
.equ HW_BLIT_CLIP_X_MIN_L, 0xFEC8
.equ HW_BLIT_CLIP_X_MIN_H, 0xFEC9
.equ HW_BLIT_CLIP_Y_MIN_L, 0xFECA
.equ HW_BLIT_CLIP_Y_MIN_H, 0xFECB
.equ HW_BLIT_CLIP_X_MAX_L, 0xFECC
.equ HW_BLIT_CLIP_X_MAX_H, 0xFECD
.equ HW_BLIT_CLIP_Y_MAX_L, 0xFECE
.equ HW_BLIT_CLIP_Y_MAX_H, 0xFECF
.equ HW_BLIT_SRC_STRIDE_L, 0xFED0
.equ HW_BLIT_SRC_STRIDE_H, 0xFED1
.equ HW_BLIT_DST_STRIDE_L, 0xFED2
.equ HW_BLIT_DST_STRIDE_H, 0xFED3
.equ HW_BLIT_WIDTH_L,      0xFED4
.equ HW_BLIT_WIDTH_H,      0xFED5
.equ HW_BLIT_HEIGHT_L,     0xFED6
.equ HW_BLIT_HEIGHT_H,     0xFED7
.equ HW_BLIT_ALPHA,        0xFEDC
.equ HW_BLIT_FLAGS,        0xFEDD
.equ HW_BLIT_COLOR,        0xFEDE
.equ HW_BLIT_CMD,          0xFEDF
.equ HW_GFX_ADDR_L,        0xFEE0
.equ HW_GFX_ADDR_H,        0xFEE1
.equ HW_GFX_DATA,          0xFEEE
.equ HW_GFX_CTRL,          0xFEEF
.equ HW_TERM_OUT,          0xFEF0
.equ HW_KBD_DATA,          0xFEF1
.equ HW_KBD_STATUS,        0xFEF2
.equ HW_BANK_SEL,          0xFEFD
.equ HW_TIMER_HZ,          0xFEFE
.equ HW_HW_CTRL,           0xFEFF

; Memory Map
.equ STACK_TOP,   0xFDFF
.equ INT_VECTOR,  0xFF00

; Interrupt IDs
.equ INT_HALT,    0x00
.equ INT_BP,      0x01
.equ INT_ILL,     0x02
.equ INT_TIMER,   0x10
.equ INT_KBD,     0x11

; Blitter Commands
.equ CMD_FILL_RECT,       0x01
.equ CMD_BLIT_MEM,        0x02
.equ CMD_BLIT_MEM_TRANS,  0x03
.equ CMD_BLIT_VRAM,       0x04
.equ CMD_BLIT_VRAM_TRANS, 0x05
.equ CMD_BLIT_XOR,        0x06
.equ CMD_LINE,            0x07

; Blitter Flags
.equ BLIT_FLAG_H_FLIP,    0x01
.equ BLIT_FLAG_V_FLIP,    0x02
.equ BLIT_FLAG_ALPHA,     0x04
.equ BLIT_FLAG_CLIP,      0x08

; GFX_CTRL Flags
.equ GFX_CTRL_AUTOINC_W,  0x10
.equ GFX_CTRL_AUTOINC_R,  0x20

; HW_CTRL Flags
.equ HW_CTRL_TIMER_EN,    0x01
.equ HW_CTRL_KBD_INT_EN,  0x02

; KBD_STATUS Flags
.equ KBD_STAT_READY,      0x01
.equ KBD_STAT_RELEASE,    0x02

; Graphics Modes - Resolutions
; 8bpp
.equ MODE0_W, 320
.equ MODE0_H, 200
.equ MODE1_W, 272
.equ MODE1_H, 180
.equ MODE2_W, 256
.equ MODE2_H, 160
.equ MODE3_W, 224
.equ MODE3_H, 140
; 4bpp
.equ MODE4_W, 448
.equ MODE4_H, 280
.equ MODE5_W, 400
.equ MODE5_H, 240
.equ MODE6_W, 320
.equ MODE6_H, 200
; 2bpp
.equ MODE7_W, 640
.equ MODE7_H, 400
.equ MODE8_W, 448
.equ MODE8_H, 280
.equ MODE9_W, 320
.equ MODE9_H, 200
; 1bpp
.equ MODE10_W, 912
.equ MODE10_H, 570
.equ MODE11_W, 640
.equ MODE11_H, 400
.equ MODE12_W, 448
.equ MODE12_H, 280
.equ MODE13_W, 320
.equ MODE13_H, 200

; Graphics Modes - VRAM Sizes
.equ MODE0_VRAM_SIZE,  0xFA00
.equ MODE1_VRAM_SIZE,  0xBF00
.equ MODE2_VRAM_SIZE,  0xA000
.equ MODE3_VRAM_SIZE,  0x7A60
.equ MODE4_VRAM_SIZE,  0xF500
.equ MODE5_VRAM_SIZE,  0xBB80
.equ MODE6_VRAM_SIZE,  0x7D00
.equ MODE7_VRAM_SIZE,  0xFA00
.equ MODE8_VRAM_SIZE,  0x7A60
.equ MODE9_VRAM_SIZE,  0x3E80
.equ MODE10_VRAM_SIZE, 0xFDD4
.equ MODE11_VRAM_SIZE, 0x7D00
.equ MODE12_VRAM_SIZE, 0x3D40
.equ MODE13_VRAM_SIZE, 0x1F40

; Register bases and offsets
.equ HW_BLIT_BASE,     0xFEC0
.equ OFS_BLIT_SRC_X_L, 0x00
.equ OFS_BLIT_SRC_X_H, 0x01
.equ OFS_BLIT_SRC_Y_L, 0x02
.equ OFS_BLIT_SRC_Y_H, 0x03
.equ OFS_BLIT_DST_X_L, 0x04
.equ OFS_BLIT_DST_X_H, 0x05
.equ OFS_BLIT_DST_Y_L, 0x06
.equ OFS_BLIT_DST_Y_H, 0x07
.equ OFS_BLIT_CLIP_X_MIN_L, 0x08
.equ OFS_BLIT_CLIP_X_MIN_H, 0x09
.equ OFS_BLIT_CLIP_Y_MIN_L, 0x0A
.equ OFS_BLIT_CLIP_Y_MIN_H, 0x0B
.equ OFS_BLIT_CLIP_X_MAX_L, 0x0C
.equ OFS_BLIT_CLIP_X_MAX_H, 0x0D
.equ OFS_BLIT_CLIP_Y_MAX_L, 0x0E
.equ OFS_BLIT_CLIP_Y_MAX_H, 0x0F
.equ OFS_BLIT_SRC_STRIDE_L, 0x10
.equ OFS_BLIT_SRC_STRIDE_H, 0x11
.equ OFS_BLIT_DST_STRIDE_L, 0x12
.equ OFS_BLIT_DST_STRIDE_H, 0x13
.equ OFS_BLIT_WIDTH_L,      0x14
.equ OFS_BLIT_WIDTH_H,      0x15
.equ OFS_BLIT_HEIGHT_L,     0x16
.equ OFS_BLIT_HEIGHT_H,     0x17
.equ OFS_BLIT_ALPHA,        0x1C
.equ OFS_BLIT_FLAGS,        0x1D
.equ OFS_BLIT_COLOR,        0x1E
.equ OFS_BLIT_CMD,          0x1F

.equ HW_GFX_BASE,     0xFEE0
.equ OFS_GFX_ADDR_L,   0x00
.equ OFS_GFX_ADDR_H,   0x01
.equ OFS_GFX_DATA,     0x0E
.equ OFS_GFX_CTRL,     0x0F

.equ HW_GP_BASE,    0xFEF0
.equ OFS_TERM_OUT,    0x00
.equ OFS_KBD_DATA,    0x01
.equ OFS_KBD_STATUS,  0x02
.equ OFS_BANK,        0x0D
.equ OFS_TIMER_HZ,    0x0E
.equ OFS_HW_CTRL,     0x0F