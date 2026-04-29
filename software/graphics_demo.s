.include "arch.s"

init_stack:
        LI      r7, >STACK_TOP
        MOV     sph, r7
        LI      r6, <STACK_TOP
        MOV     spl, r6

        BL      AL, clear_screen
        BL      AL, static_gradient
        BL      AL, moving_gradient
        BL      AL, moving_dot
        BL      AL, draw_sprites
wait_loop:
        B       AL, wait_loop

clear_screen:
        LI      r7, >VRAM_START
        LI      r6, <VRAM_START
        LI      r5, >VRAM_SIZE
        LI      r4, <VRAM_SIZE
        LI      r0, 0x00
clear_loop:
        SB      r0, [a3]
        ADD     r6, r6, 1
        INCC    r7
        ADD     r4, r4, -1
        DECB    r5
        B       ZC, clear_loop
        CMA     r4, r4
        B       ZC, clear_loop
        RET

static_gradient:
        LI      r7, >VRAM_START
        LI      r6, <VRAM_START
        LI      r5, >VRAM_SIZE
        LI      r4, <VRAM_SIZE
static_gradient_loop:
        SB      r0, [a3]
        ADD     r0, r0, 1
        ADD     r6, r6, 1
        INCC    r7
        ADD     r4, r4, -1
        DECB    r5
        B       ZC, static_gradient_loop
        CMA     r4, r4
        B       ZC, static_gradient_loop
        LI      r4, 0x08
        LI      r5, 0xFF
        LI      r6, 0xFF
        LI      r7, 0xFF
static_gradient_wait_loop:
        ADD     r7, r7, -1
        DECB    r6
        DECB    r5
        DECB    r4
        B       ZC, static_gradient_wait_loop
        CMA     r5, r5
        B       ZC, static_gradient_wait_loop
        CMA     r6, r6
        B       ZC, static_gradient_wait_loop
        CMA     r7, r7
        B       ZC, static_gradient_wait_loop
        RET

moving_gradient:
        LI      r3, 0xFF
        LI      r2, 0x00
moving_gradient_loop_outerer:
        LI      r7, 0xFF
        LI      r6, 0xFF
        LI      r5, 0x20
moving_gradient_loop_outer:
        ADD     r7, r7, -1
        DECB    r6
        DECB    r5
        B       ZC, moving_gradient_loop_outer
        CMA     r6, r6
        B       ZC, moving_gradient_loop_outer
        CMA     r7, r7
        B       ZC, moving_gradient_loop_outer
        LI      r7, >VRAM_START
        LI      r6, <VRAM_START
        LI      r5, >VRAM_SIZE
        LI      r4, <VRAM_SIZE
        ADD     r0, r0, 1
moving_gradient_loop_inner:
        SB      r0, [a3]
        ADD     r0, r0, 1
        ADD     r6, r6, 1
        INCC    r7
        ADD     r4, r4, -1
        DECB    r5
        B       ZC, moving_gradient_loop_inner
        CMA     r4, r4
        B       ZC, moving_gradient_loop_inner
        ADD     r2, r2, 1
        INCC    r3
        LI      r1, 0xFF
        CMP     r1, r2
        B       NE, moving_gradient_loop_outerer
        CMP     r1, r3
        B       NE, moving_gradient_loop_outerer
        RET

moving_dot:
        BL      AL, clear_screen
        LI      r7, >VRAM_START
        LI      r6, <VRAM_START
        LI      r0, 0x00
moving_dot_loop_outer:
        LI      r2, 0xFF
        LI      r3, 0xFF
        LI      r4, 0x04
        SB      r0, [a3]
        ADD     r6, r6, 1
        INCC    r7
        ADD     r1, r1, 1
        SB      r1, [a3]
moving_dot_loop_inner:
        ADD     r2, r2, -1
        DECB    r3
        DECB    r4
        B       ZC, moving_dot_loop_inner
        CMA     r3, r3
        B       ZC, moving_dot_loop_inner
        CMA     r2, r2
        B       ZC, moving_dot_loop_inner
        LI      r2, 0xD0
        CMP     r2, r7
        B       NE, moving_dot_loop_outer
        RET

draw_sprites:
        LI      r1, >sprite0
        LI      r0, <sprite0
        LI      r2, 80          ; x
        LI      r3, 80          ; y
        LI      r4, 8           ; width
        LI      r5, 8           ; height
        BL      AL, draw_sprite
        LI      r1, >sprite1
        LI      r0, <sprite1
        LI      r2, 2
        LI      r3, 2
        LI      r4, 8
        LI      r5, 8
        BL      AL, draw_sprite
        LI      r1, >sprite2
        LI      r0, <sprite2
        LI      r2, 2          ; x
        LI      r3, 80          ; y
        LI      r4, 8           ; width
        LI      r5, 8           ; height
        BL      AL, draw_sprite
        LI      r1, >sprite3
        LI      r0, <sprite3
        LI      r2, 80          ; x
        LI      r3, 2          ; y
        LI      r4, 8           ; width
        LI      r5, 8           ; height
        BL      AL, draw_sprite
        RET

draw_sprite:
        PUSH    r0
        PUSH    r1
        ; calculate vram destination
        ; formula: VRAM_START + y * 90 + x
        LI      r7, >VRAM_START
        LI      r6, <VRAM_START
        ; add x
        ADD     r6, r6, r2
        INCC    r7
draw_sprite_y_offset:
        LI      r0, 0
        LI      r1, 90
draw_sprite_y_offset_loop:
        ; add y * 90
        CMP     r0, r3
        B       EQ, draw_sprite_blitter
        ADD     r6, r6, r1
        INCC    r7
        ADD     r3, r3, -1
        B       AL, draw_sprite_y_offset_loop

draw_sprite_blitter:
        POP     r3              ; restore source destination pointer
        POP     r2
        MOV     r0, r4          ; width
        MOV     r1, r5          ; height
draw_sprite_blitter_loop:
        PUSH    r0
        LB      r0, [a1]
        SB      r0, [a3]
        POP     r0
        ADD     r6, r6, 1
        INCC    r7
        ADD     r2, r2, 1
        INCC    r3
        ADD     r0, r0, -1
        B       ZC, draw_sprite_blitter_loop
        LI      r0, 90
        SUB     r0, r0, r4
        ADD     r6, r6, r0
        INCC    r7
        MOV     r0, r4
        ADD     r1, r1, -1
        B       ZC, draw_sprite_blitter_loop
        RET

        


        

        

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