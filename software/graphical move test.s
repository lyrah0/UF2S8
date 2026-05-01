.include "arch.s"

; Screen dimensions: 256x256, 8bpp (GFX_WIDTH/HEIGHT use units of 8 pixels)
.equ SCREEN_W_UNITS, 32          ; 32 * 8 = 256
.equ SCREEN_H_UNITS, 32          ; 32 * 8 = 256
.equ DOT_SIZE,       4           ; 4x4 pixel dot

init_stack:
        LI      r7, >STACK_TOP
        MOV     sph, r7
        LI      r6, <STACK_TOP
        MOV     spl, r6

init_interrupts:
        ; Write kbd_isr address into vector table slot for INT_KBD (0x11)
        ; Vector = INT_VECTOR + INT_KBD * 2 = 0xFF00 + 0x22
        LI      r7, >INT_VECTOR
        LI      r6, <INT_VECTOR
        LI      r0, >kbd_isr
        SB      r0, [a3+0x23]
        LI      r0, <kbd_isr
        SB      r0, [a3+0x22]

        ; Enable keyboard interrupts
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, HW_CTRL_KBD_INT_EN
        SB      r0, [a3+OFS2_HW_CTRL]

init_graphics:
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, SCREEN_W_UNITS
        SB      r0, [a3+OFS2_GFX_WIDTH]
        LI      r0, SCREEN_H_UNITS
        SB      r0, [a3+OFS2_GFX_HEIGHT]
        LI      r0, 0x00                ; 8bpp mode
        SB      r0, [a3+OFS2_GFX_CTRL]

init_flags:
        LI      r0, 0x80
        MOV     flags, r0

; --- Clear the screen with CMD_FILL_RECT ---
clear_screen:
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        LI      r0, 0x00
        SB      r0, [a3+OFS2_BLIT_DST_X_L]
        SB      r0, [a3+OFS2_BLIT_DST_X_H]
        SB      r0, [a3+OFS2_BLIT_DST_Y_L]
        SB      r0, [a3+OFS2_BLIT_DST_Y_H]
        SB      r0, [a3+OFS2_BLIT_COLOR]       ; fill color = 0 (black)
        LI      r0, 0x00
        SB      r0, [a3+OFS2_BLIT_WIDTH_H]
        SB      r0, [a3+OFS2_BLIT_HEIGHT_H]
        LI      r0, 0xFF                        ; width = 255
        SB      r0, [a3+OFS2_BLIT_WIDTH_L]
        LI      r0, 0xFF                        ; height = 255
        SB      r0, [a3+OFS2_BLIT_HEIGHT_L]
        LI      r0, CMD_FILL_RECT
        SB      r0, [a3+OFS2_BLIT_CMD]

; --- Main game loop ---
game_loop:
        ; Draw the initial dot at the stored position
        LI      r7, >player_x
        LI      r6, <player_x
        LB      r2, [a3]        ; x
        LB      r3, [a3+1]      ; y
        LI      r0, 0xFF        ; white
        BL      AL, draw_dot

wait_for_input:
        WFI                     ; sleep until interrupt fires

        ; Check moved flag
        LI      r7, >moved
        LI      r6, <moved
        LB      r0, [a3]
        CMA     r0, r0
        B       EQ, wait_for_input

        ; Reset moved flag
        LI      r0, 0
        SB      r0, [a3]

        ; Erase old dot (draw in black)
        LI      r7, >player_x
        LI      r6, <player_x
        LB      r2, [a3]        ; old x
        LB      r3, [a3+1]      ; old y
        LI      r0, 0x00        ; black
        BL      AL, draw_dot

        ; Apply delta: x += dx, y += dy  (wraps 0-255)
        LI      r7, >next_dx
        LI      r6, <next_dx
        LB      r4, [a3]        ; dx
        LB      r5, [a3+1]      ; dy

        LI      r7, >player_x
        LI      r6, <player_x
        LB      r2, [a3]
        LB      r3, [a3+1]
        ADD     r2, r2, r4      ; x += dx
        ADD     r3, r3, r5      ; y += dy
        SB      r2, [a3]
        SB      r3, [a3+1]

        ; Draw new dot
        LI      r0, 0xFF        ; white
        BL      AL, draw_dot

        B       AL, wait_for_input


; --- draw_dot subroutine ---
; Inputs: r2 = x, r3 = y, r0 = color
; Uses the blitter CMD_FILL_RECT to draw a DOT_SIZE x DOT_SIZE square
; Clobbers: r1, r4, r5, r6, r7
draw_dot:
        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2
        SB      r0, [a3+OFS2_BLIT_COLOR]
        SB      r2, [a3+OFS2_BLIT_DST_X_L]
        LI      r0, 0
        SB      r0, [a3+OFS2_BLIT_DST_X_H]
        SB      r3, [a3+OFS2_BLIT_DST_Y_L]
        SB      r0, [a3+OFS2_BLIT_DST_Y_H]
        LI      r0, DOT_SIZE
        SB      r0, [a3+OFS2_BLIT_WIDTH_L]
        LI      r0, 0
        SB      r0, [a3+OFS2_BLIT_WIDTH_H]
        LI      r0, DOT_SIZE
        SB      r0, [a3+OFS2_BLIT_HEIGHT_L]
        SB      r0, [a3+OFS2_BLIT_HEIGHT_H]  ; r0 is still DOT_SIZE, reuse for height_h would be wrong; zero it
        LI      r0, 0
        SB      r0, [a3+OFS2_BLIT_HEIGHT_H]
        LI      r0, CMD_FILL_RECT
        SB      r0, [a3+OFS2_BLIT_CMD]
        RET


; --- Keyboard ISR ---
kbd_isr:
        ; Save registers clobbered by the ISR
        PUSH    r0
        PUSH    r1
        PUSH    r4
        PUSH    r5
        PUSH    r6
        PUSH    r7

        LI      r7, >HW_BASE2
        LI      r6, <HW_BASE2

        ; Read status, then always consume the data register
        LB      r0, [a3+OFS2_KBD_STATUS]
        PUSH    r0                              ; save status
        LB      r0, [a3+OFS2_KBD_DATA]         ; consume event

        ; Skip if it was a key release
        POP     r1
        LI      r4, KBD_STAT_RELEASE
        AND     r1, r1, r4
        CMP     r1, r4
        B       EQ, isr_done

        ; Reset deltas
        LI      r4, 0
        LI      r5, 0

        ; Check Up (SDL 0x52 / 'w' 0x77)
        LI      r1, 0x52
        CMP     r0, r1
        B       EQ, do_up
        LI      r1, 0x77
        CMP     r0, r1
        B       EQ, do_up

        ; Check Down (SDL 0x51 / 's' 0x73)
        LI      r1, 0x51
        CMP     r0, r1
        B       EQ, do_down
        LI      r1, 0x73
        CMP     r0, r1
        B       EQ, do_down

        ; Check Left (SDL 0x50 / 'a' 0x61)
        LI      r1, 0x50
        CMP     r0, r1
        B       EQ, do_left
        LI      r1, 0x61
        CMP     r0, r1
        B       EQ, do_left

        ; Check Right (SDL 0x4F / 'd' 0x64)
        LI      r1, 0x4F
        CMP     r0, r1
        B       EQ, do_right
        LI      r1, 0x64
        CMP     r0, r1
        B       EQ, do_right

        B       AL, isr_done

do_up:   LI      r5, -1
        B       AL, store_delta
do_down: LI      r5, 1
        B       AL, store_delta
do_left: LI      r4, -1
        B       AL, store_delta
do_right: LI     r4, 1

store_delta:
        LI      r7, >next_dx
        LI      r6, <next_dx
        SB      r4, [a3]        ; dx
        SB      r5, [a3+1]      ; dy
        LI      r0, 1
        LI      r6, <moved
        SB      r0, [a3]        ; set moved flag

isr_done:
        POP     r7
        POP     r6
        POP     r5
        POP     r4
        POP     r1
        POP     r0
        RETI


; --- Data Section ---
player_x: .db 0x80
player_y: .db 0x80
next_dx:  .db 0x00
next_dy:  .db 0x00
moved:    .db 0x00