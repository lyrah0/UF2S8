.include "arch.s"

init_stack:
        LI      r7, >STACK_TOP
        MOV     sph, r7
        LI      r6, <STACK_TOP
        MOV     spl, r6
init_timer:
        LI      r7, >INT_VECTOR
        LI      r6, <INT_VECTOR
        LI      r0, >int_handler
        SB      r0, [a3+0x21]
        LI      r0, <int_handler
        SB      r0, [a3+0x20]
        
        LI      r7, >HW_GP_BASE
        LI      r6, <HW_GP_BASE
        LI      r0, 0x01
        SB      r0, [a3+OFS_HW_CTRL]
        LI      r0, 10
        SB      r0, [a3+OFS_TIMER_HZ]

        LI      r0, 0x80
        MOV     flags, r0
init_graphics:
        LI      r0, 40                  ; 320
        SB      r0, [a3+OFS_GFX_WIDTH]
        LI      r0, 25                  ; 200
        SB      r0, [a3+OFS_GFX_HEIGHT]
        ;LI      r0, 0x00                ; mode 0
        ;SB      r0, [a3+OFS_GFX_CTRL]

demos_start:
        BL      AL, clear_screen
        LI      r3, 0x00                ; don't skip vsync wait
        BL      AL, draw_rects
        BL      AL, draw_sprites
        LI      r2, 40                  ; wait 40 frames
        BL      AL, wait
        LI      r3, 0x01                ; skip wait
        BL      AL, draw_rects
idle_loop:
        WFI
        B       AL, idle_loop

clear_screen:
        LI      r7, >HW_BLIT_BASE
        LI      r6, <HW_BLIT_BASE
        LI      r0, 0x00
        SB      r0, [a3+OFS_BLIT_DST_X_L]
        SB      r0, [a3+OFS_BLIT_DST_X_H]
        SB      r0, [a3+OFS_BLIT_DST_Y_L]
        SB      r0, [a3+OFS_BLIT_DST_Y_H]
        SB      r0, [a3+OFS_BLIT_COLOR]
        LI      r0, 0x40
        SB      r0, [a3+OFS_BLIT_WIDTH_L]
        LI      r0, 0x01
        SB      r0, [a3+OFS_BLIT_WIDTH_H]
        LI      r0, 0xC8
        SB      r0, [a3+OFS_BLIT_HEIGHT_L]
        LI      r0, 0x00
        SB      r0, [a3+OFS_BLIT_HEIGHT_H]
        LI      r0, CMD_FILL_RECT
        SB      r0, [a3+OFS_BLIT_CMD]
        RET


draw_rects:
        LI      r7, >HW_BLIT_BASE
        LI      r6, <HW_BLIT_BASE
        LI      r0, 0x00
        SB      r0, [a3+OFS_BLIT_DST_X_H]
        SB      r0, [a3+OFS_BLIT_DST_Y_H]
        SB      r0, [a3+OFS_BLIT_WIDTH_H]
        SB      r0, [a3+OFS_BLIT_HEIGHT_H]
        LI      r0, 40          ; width
        SB      r0, [a3+OFS_BLIT_WIDTH_L]
        LI      r0, 64          ; height
        SB      r0, [a3+OFS_BLIT_HEIGHT_L]
        LI      r4, 0x00        ; x
        LI      r5, 0x00        ; y
        LI      r1, 40          ; number of rects to draw
draw_rects_loop:
        SB      r0, [a3+OFS_BLIT_COLOR]
        ADD     r0, r0, 17
        SB      r4, [a3+OFS_BLIT_DST_X_L]
        SB      r5, [a3+OFS_BLIT_DST_Y_L]
        ADD     r4, r4, 5
        ADD     r5, r5, 3
        PUSH    r0
        LI      r0, CMD_FILL_RECT
        SB      r0, [a3+OFS_BLIT_CMD]
        POP     r0
        CMA     r1, r1
        B       ZS, draw_rects_end
        ADD     r1, r1, -1
        CMA     r3, r3
        B       ZC, draw_rects_skip
        WFI
draw_rects_skip:
        B       AL, draw_rects_loop
draw_rects_end:
        RET


draw_sprites:
        LI      r7, >HW_BLIT_BASE
        LI      r6, <HW_BLIT_BASE
        LI      r0, 0x00
        SB      r0, [a3+OFS_BLIT_DST_X_H]
        SB      r0, [a3+OFS_BLIT_DST_Y_H]
        SB      r0, [a3+OFS_BLIT_WIDTH_H]
        SB      r0, [a3+OFS_BLIT_HEIGHT_H]
        LI      r0, 8          ; width
        SB      r0, [a3+OFS_BLIT_WIDTH_L]
        LI      r0, 8          ; height
        SB      r0, [a3+OFS_BLIT_HEIGHT_L]
        LI      r4, 0x20        ; x
        SB      r4, [a3+OFS_BLIT_DST_X_L]
        LI      r5, 0x10        ; y
        SB      r5, [a3+OFS_BLIT_DST_Y_L]
        LI      r0, 0x00        ; color
        SB      r0, [a3+OFS_BLIT_COLOR]
        LI      r0, >sprite0
        SB      r0, [a3+OFS_BLIT_SRC_X_H]
        LI      r0, <sprite0
        SB      r0, [a3+OFS_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS_BLIT_CMD]
        LI      r4, 0x30        ; x
        SB      r4, [a3+OFS_BLIT_DST_X_L]
        LI      r5, 0x18        ; y
        SB      r5, [a3+OFS_BLIT_DST_Y_L]
        LI      r0, >sprite1
        SB      r0, [a3+OFS_BLIT_SRC_X_H]
        LI      r0, <sprite1
        SB      r0, [a3+OFS_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS_BLIT_CMD]
        LI      r4, 0x40        ; x
        SB      r4, [a3+OFS_BLIT_DST_X_L]
        LI      r5, 0x20        ; y
        SB      r5, [a3+OFS_BLIT_DST_Y_L]
        LI      r0, >sprite2
        SB      r0, [a3+OFS_BLIT_SRC_X_H]
        LI      r0, <sprite2
        SB      r0, [a3+OFS_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS_BLIT_CMD]
        LI      r4, 0x50        ; x
        SB      r4, [a3+OFS_BLIT_DST_X_L]
        LI      r5, 0x28        ; y
        SB      r5, [a3+OFS_BLIT_DST_Y_L]
        LI      r0, >sprite3
        SB      r0, [a3+OFS_BLIT_SRC_X_H]
        LI      r0, <sprite3
        SB      r0, [a3+OFS_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS_BLIT_CMD]
draw_sprites_end:
        RET

wait:
        WFI
        ADD     r2, r2, -1
        B       ZC, wait
        RET

.bankw1 0
.origin 0x8000
sprite0:
.db 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
.db 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00
.db 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00

sprite1:
.db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF
.db 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF
.db 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF
.db 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF

sprite2:
.db 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
.db 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00
.db 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF
.db 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF
.db 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00
.db 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00

sprite3:
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
.db 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00
.db 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00
.db 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00
.db 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00
.db 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00
.db 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00
.db 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF

.origin 0xC000
int_handler:
        RETI