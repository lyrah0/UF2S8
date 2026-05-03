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

        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, 0x01
        SB      r0, [a3+OFS2_HW_CTRL]
        LI      r0, 10
        SB      r0, [a3+OFS2_TIMER_HZ]

        LI      r0, 0x80
        MOV     flags, r0
init_graphics:
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, 40                  ; 320
        SB      r0, [a3+OFS2_GFX_WIDTH]
        LI      r0, 25                  ; 200
        SB      r0, [a3+OFS2_GFX_HEIGHT]
        LI      r0, 0x00                ; mode 0
        SB      r0, [a3+OFS2_GFX_CTRL]

demos_start:
        BL      AL, clear_screen
        LI      r3, 0x00                ; don't skip vsync wait
        BL      AL, draw_rects
        BL      AL, draw_sprites
        LI      r2, 40                  ; wait 40 frames
        BL      AL, wait
        LI      r3, 0x01                ; skip wait
        BL      AL, draw_rects
        BL      AL, print_chars
idle_loop:
        WFI
        B       AL, idle_loop

clear_screen:
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, 0x00
        SB      r0, [a3+OFS2_BLIT_DST_X_L]
        SB      r0, [a3+OFS2_BLIT_DST_X_H]
        SB      r0, [a3+OFS2_BLIT_DST_Y_L]
        SB      r0, [a3+OFS2_BLIT_DST_Y_H]
        SB      r0, [a3+OFS2_BLIT_COLOR]
        LI      r0, 0x40
        SB      r0, [a3+OFS2_BLIT_WIDTH_L]
        LI      r0, 0x01
        SB      r0, [a3+OFS2_BLIT_WIDTH_H]
        LI      r0, 0xC8
        SB      r0, [a3+OFS2_BLIT_HEIGHT_L]
        LI      r0, 0x00
        SB      r0, [a3+OFS2_BLIT_HEIGHT_H]
        LI      r0, CMD_FILL_RECT
        SB      r0, [a3+OFS2_BLIT_CMD]
        RET


draw_rects:
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, 0x00
        SB      r0, [a3+OFS2_BLIT_DST_X_H]
        SB      r0, [a3+OFS2_BLIT_DST_Y_H]
        SB      r0, [a3+OFS2_BLIT_WIDTH_H]
        SB      r0, [a3+OFS2_BLIT_HEIGHT_H]
        LI      r0, 40          ; width
        SB      r0, [a3+OFS2_BLIT_WIDTH_L]
        LI      r0, 64          ; height
        SB      r0, [a3+OFS2_BLIT_HEIGHT_L]
        LI      r4, 0x00        ; x
        LI      r5, 0x00        ; y
        LI      r1, 40          ; number of rects to draw
draw_rects_loop:
        SB      r0, [a3+OFS2_BLIT_COLOR]
        ADD     r0, r0, 17
        SB      r4, [a3+OFS2_BLIT_DST_X_L]
        SB      r5, [a3+OFS2_BLIT_DST_Y_L]
        ADD     r4, r4, 5
        ADD     r5, r5, 3
        PUSH    r0
        LI      r0, CMD_FILL_RECT
        SB      r0, [a3+OFS2_BLIT_CMD]
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
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, 0x00
        SB      r0, [a3+OFS2_BLIT_DST_X_H]
        SB      r0, [a3+OFS2_BLIT_DST_Y_H]
        SB      r0, [a3+OFS2_BLIT_WIDTH_H]
        SB      r0, [a3+OFS2_BLIT_HEIGHT_H]
        LI      r0, 8          ; width
        SB      r0, [a3+OFS2_BLIT_WIDTH_L]
        LI      r0, 8          ; height
        SB      r0, [a3+OFS2_BLIT_HEIGHT_L]
        LI      r4, 0x20        ; x
        SB      r4, [a3+OFS2_BLIT_DST_X_L]
        LI      r5, 0x10        ; y
        SB      r5, [a3+OFS2_BLIT_DST_Y_L]
        LI      r0, 0x00        ; color
        SB      r0, [a3+OFS2_BLIT_COLOR]
        LI      r0, >sprite0
        SB      r0, [a3+OFS2_BLIT_SRC_X_H]
        LI      r0, <sprite0
        SB      r0, [a3+OFS2_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS2_BLIT_CMD]
        LI      r4, 0x30        ; x
        SB      r4, [a3+OFS2_BLIT_DST_X_L]
        LI      r5, 0x18        ; y
        SB      r5, [a3+OFS2_BLIT_DST_Y_L]
        LI      r0, >sprite1
        SB      r0, [a3+OFS2_BLIT_SRC_X_H]
        LI      r0, <sprite1
        SB      r0, [a3+OFS2_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS2_BLIT_CMD]
        LI      r4, 0x40        ; x
        SB      r4, [a3+OFS2_BLIT_DST_X_L]
        LI      r5, 0x20        ; y
        SB      r5, [a3+OFS2_BLIT_DST_Y_L]
        LI      r0, >sprite2
        SB      r0, [a3+OFS2_BLIT_SRC_X_H]
        LI      r0, <sprite2
        SB      r0, [a3+OFS2_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS2_BLIT_CMD]
        LI      r4, 0x50        ; x
        SB      r4, [a3+OFS2_BLIT_DST_X_L]
        LI      r5, 0x28        ; y
        SB      r5, [a3+OFS2_BLIT_DST_Y_L]
        LI      r0, >sprite3
        SB      r0, [a3+OFS2_BLIT_SRC_X_H]
        LI      r0, <sprite3
        SB      r0, [a3+OFS2_BLIT_SRC_X_L]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a3+OFS2_BLIT_CMD]
draw_sprites_end:
        RET

wait:
        WFI
        ADD     r2, r2, -1
        B       ZC, wait
        RET

print_char:
        PUSH    a3
        MOV     r6, spl
        MOV     r7, sph

        LI      r5, >HW_BASE2
        LI      r4, <HW_BASE2

        SB      r0, [a2+OFS2_BLIT_DST_X_L]
        SB      r1, [a2+OFS2_BLIT_DST_X_H]
        SB      r2, [a2+OFS2_BLIT_DST_Y_L]
        LI      r0, 0x00
        SB      r0, [a2+OFS2_BLIT_DST_Y_H]
        SB      r0, [a2+OFS2_BLIT_SRC_STRIDE_H]
        SB      r0, [a2+OFS2_BLIT_WIDTH_H]
        SB      r0, [a2+OFS2_BLIT_HEIGHT_H]
        SB      r0, [a2+OFS2_BLIT_COLOR]
        LI      r0, 0x80
        SB      r0, [a2+OFS2_BLIT_SRC_STRIDE_L]
        LI      r0, 0x08
        SB      r0, [a2+OFS2_BLIT_WIDTH_L]
        SB      r0, [a2+OFS2_BLIT_HEIGHT_L]

        LB      r2, [a3+5]      ; src x
        LI      r3, 0
        LI      r0, 8
        LI      r1, 0
        BL      AL, multiply

        PUSH    a0
        LB      r2, [a3+6]      ; src y
        LI      r3, 0
        LI      r0, 0x00
        LI      r1, 0x04
        BL      AL, multiply

        POP     a1
        ADD     r0, r0, r2
        ADC     r1, r1, r3

        LI      r3, >font_8x8
        LI      r2, <font_8x8
        ADD     r0, r0, r2
        ADC     r1, r1, r3
        
        LI      r5, >HW_BASE2
        LI      r4, <HW_BASE2
        SB      r0, [a2+OFS2_BLIT_SRC_X_L]
        SB      r1, [a2+OFS2_BLIT_SRC_X_H]
        LI      r0, CMD_BLIT_MEM_TRANS
        SB      r0, [a2+OFS2_BLIT_CMD]
print_char_end:
        MOV     spl, r6
        MOV     sph, r7
        POP     a3
        RET




multiply:
        PUSH    a3
        MOV     r6, spl
        MOV     r7, sph

        MOV     r4, r0
        MOV     r5, r1

        LI      r0, 0
        LI      r1, 0

multiply_loop:
        CMA     r2, r2
        B       ZC, multiply_continue
        CMA     r3, r3
        B       ZS, multiply_end
multiply_continue:
        ADD     r2, r2, -1
        DECB    r3
        ADD     r0, r0, r4
        ADC     r1, r1, r5
        B       AL, multiply_loop
multiply_end:
        MOV     spl, r6
        MOV     sph, r7
        POP     a3
        RET


print_chars:
        ; H
        LI      r0, 7
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x00
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; E
        LI      r0, 4
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x08
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; L
        LI      r0, 11
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x10
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; L
        LI      r0, 11
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x18
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; O
        LI      r0, 14
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x20
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; W
        LI      r0, 6
        LI      r1, 1
        PUSH    a0
        LI      r0, 0x30
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; O
        LI      r0, 14
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x38
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; R
        LI      r0, 1
        LI      r1, 1
        PUSH    a0
        LI      r0, 0x40
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; L
        LI      r0, 11
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x48
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
        ; D
        LI      r0, 3
        LI      r1, 0
        PUSH    a0
        LI      r0, 0x50
        LI      r1, 0x00
        LI      r2, 0x00
        BL      AL, print_char
        WFI
        POP     a0
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

.include "8x8font.s"

.origin 0xC000
int_handler:
        RETI