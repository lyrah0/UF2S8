.equ STACK_TOP, 0xFDFF
.equ VRAM_START, 0x8000
.equ VRAM_SIZE, 0x4000
.equ KBD_INT_VECTOR, 0xFF06
.equ KBD_STATUS, 0xFEF2
.equ KBD_DATA, 0xFEF1
.equ HW_CONTROL, 0xFEFF
.equ GRAPHICS_CONTROL, 0xFEFD
.equ INITIAL_POS, 0x00B4

init_stack:
        LI      r0, >STACK_TOP
        MOV     sph, r0
        LI      r0, <STACK_TOP
        MOV     spl, r0
init_interrupts:
        LI	r7, >kbd_isr
        LI	r6, <kbd_isr
        LI      r5, >KBD_INT_VECTOR
        LI      r4, <KBD_INT_VECTOR
        SB      r6, [a2]
        ADD     r4, r4, 1
        SB      r7, [a2]
        LI      r7, >HW_CONTROL
        LI      r6, <HW_CONTROL
        LI      r0, 0x02        ; enable keyboard interrupts
        SB      r0, [a3]
init_graphics:
        LI      r7, >GRAPHICS_CONTROL
        LI      r6, <GRAPHICS_CONTROL
        LI      r0, 0x00        ; mode 0 = 1bpp
        SB      r0, [a3]
        LI      r6, 0xE0
        LI      r0, 0x00        ; black
        SB      r0, [a3]        ; store 0(black) in palette(0)
        LI      r0, 0xFF        ; white
        SB      r0, [a3+1]      ; store 255(white) in palette(1)
init_flags:
        LI      r0, 0x80
        MOV     flags, r0



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

game_loop:
        ; Draw initial dot
        LI      r7, >player_y_l
        LI      r6, <player_y_l
        LB      r4, [a3]
        LB      r5, [a3+1]
        
        LI      r7, >player_x_l
        LI      r6, <player_x_l
        LB      r2, [a3]
        LB      r3, [a3+1]
        
        LI      r0, 1           ; mode 1=set
        BL      AL, draw_pixel

wait_for_input:
        ; Poll the 'moved' flag
        LI      r7, >moved
        LI      r6, <moved
        LB      r0, [a3]
        CMA     r0, r0
        B       EQ, wait_for_input
        
        ; Reset moved flag
        LI      r0, 0
        SB      r0, [a3]

        ; Clear old pixel
        LI      r7, >player_y_l
        LI      r6, <player_y_l
        LB      r4, [a3]
        LB      r5, [a3+1]
        LI      r7, >player_x_l
        LI      r6, <player_x_l
        LB      r2, [a3]
        LB      r3, [a3+1]
        LI      r0, 0           ; mode 0=clear
        BL      AL, draw_pixel

        ; Update X
        LI      r7, >next_dx
        LI      r6, <next_dx
        LB      r2, [a3]
        
        LI      r7, >player_x_l
        LI      r6, <player_x_l
        LB      r0, [a3]
        LB      r1, [a3+1]
        LI      r4, 0
        CMA     r2, r2
        B       PL, upd_x_pos
        LI      r4, 0xFF
upd_x_pos:
        ADD     r0, r0, r2
        ADC     r1, r1, r4
        
        ; Bounds check X -> clamp 0 to 359
        CMA     r1, r1
        B       PL, upd_x_max
        LI      r0, 0
        LI      r1, 0
        B       AL, upd_x_done
upd_x_max:
        LI      r4, 1
        CMP     r1, r4
        B       LO, upd_x_done
        B       EQ, check_low_x
        B       AL, clamp_x
check_low_x:
        LI      r4, 0x68
        CMP     r0, r4
        B       LO, upd_x_done
clamp_x:
        LI      r0, 0x67
        LI      r1, 0x01
upd_x_done:
        SB      r0, [a3]
        SB      r1, [a3+1]

        ; Update Y
        LI      r7, >next_dy
        LI      r6, <next_dy
        LB      r2, [a3]

        LI      r7, >player_y_l
        LI      r6, <player_y_l
        LB      r0, [a3]
        LB      r1, [a3+1]
        LI      r4, 0
        CMA     r2, r2
        B       PL, upd_y_pos
        LI      r4, 0xFF
upd_y_pos:
        ADD     r0, r0, r2
        ADC     r1, r1, r4
        
        ; Bounds check Y < 360
        CMA     r1, r1
        B       PL, upd_y_max
        LI      r0, 0
        LI      r1, 0
        B       AL, upd_y_done
upd_y_max:
        LI      r4, 1
        CMP     r1, r4
        B       LO, upd_y_done
        B       EQ, check_low_y
        B       AL, clamp_y
check_low_y:
        LI      r4, 0x68
        CMP     r0, r4
        B       LO, upd_y_done
clamp_y:
        LI      r0, 0x67
        LI      r1, 0x01
upd_y_done:
        SB      r0, [a3]
        SB      r1, [a3+1]

        ; Now set up for draw_pixel
        LI      r7, >player_x_l
        LI      r6, <player_x_l
        LB      r2, [a3]
        LB      r3, [a3+1]
        LI      r7, >player_y_l
        LI      r6, <player_y_l
        LB      r4, [a3]
        LB      r5, [a3+1]
        
        ; Draw new pixel
        LI      r0, 1           ; mode 1=set
        BL      AL, draw_pixel
        B       AL, wait_for_input

; --- draw_pixel routine ---
; Inputs: r3:r2 = X, r5:r4 = Y, r0 = color (0 or 1)
; Clobbers: r1-r7
draw_pixel:
        PUSH    r0              ; save color
        
        ; Calculate Y * 45 into r7:r6
        LI      r6, 0   ; result low
        LI      r7, 0   ; result high
        LI      r1, 45  ; counter
dp_mul_loop:
        CMA     r1, r1
        B       EQ, dp_mul_done
        ADD     r6, r6, r4  ; result_low += Y_low
        ADC     r7, r7, r5  ; result_high += Y_high
        
        PUSH    r0
        LI      r0, -1
        ADD     r1, r1, r0  ; r1 -= 1
        POP     r0
        B       AL, dp_mul_loop
dp_mul_done:

        ; r7:r6 now has Y * 45. Add VRAM base 0x80 to r7
        PUSH    r1
        LI      r1, 0x80
        ADD     r7, r7, r1
        POP     r1
        
        ; Calculate X / 8
        MOV     r1, r2       ; r1 = X low
        SRL     r1, r1, 3    ; r1 = X low >> 3
        CMA     r3, r3       ; is X high 0?
        B       EQ, dp_x_div_8_done
        PUSH    r0
        LI      r0, 32
        ADD     r1, r1, r0   ; +32
        POP     r0
dp_x_div_8_done:
        
        ; Add (X / 8) to r7:r6 (a3 is addressed)
        ADD     r6, r6, r1
        PUSH    r1
        LI      r1, 0
        ADC     r7, r7, r1  ; add carry
        POP     r1
        
        ; X % 8 into r1
        MOV     r1, r2       ; r1 = X low
        PUSH    r0
        LI      r0, 7
        AND     r1, r1, r0   ; r1 = X % 8
        LI      r0, 7
        SUB     r1, r0, r1   ; r1 = 7 - (X % 8) (shift count)
        POP     r0
        
        ; Create mask into r4
        LI      r4, 1
        SLL     r4, r4, r1   ; r4 = 1 << shift count
        
        ; Read byte from VRAM
        LB      r1, [a3]
        
        POP     r0           ; restore color
        CMA     r0, r0
        B       EQ, dp_clear
dp_set:
        OR      r1, r1, r4
        B       AL, dp_store
dp_clear:
        NOR     r4, r4, r4   ; invert mask
        AND     r1, r1, r4
dp_store:
        SB      r1, [a3]
        RET


kbd_isr:
        ; Save registers
        PUSH    r0
        PUSH    r1
        PUSH    r4
        PUSH    r5
        PUSH    r6
        PUSH    r7
        
        ; Switch to hardware registers address
        LI      r7, >KBD_STATUS
        LI      r6, <KBD_STATUS
        LB      r0, [a3]        ; Get status
        PUSH    r0              ; Save status for later
        
        LI      r6, <KBD_DATA
        LB      r0, [a3]        ; READ DATA (This consumes the event!)
        
        POP     r1              ; Get status back into r1
        LI      r6, 0x02        ; Release bit mask
        AND     r1, r1, r6
        CMP     r1, r6
        B       EQ, isr_done    ; If it was a release, we're done (but we consumed the byte!)
        
        ; Reset deltas
        LI      r4, 0
        LI      r5, 0

        ; Check Up (SDL 0x52, ASCII 'w' 0x77, Alt 0x41)
        LI      r1, 0x52
        CMP     r0, r1
        B       EQ, up
        LI      r1, 0x77
        CMP     r0, r1
        B       EQ, up
        LI      r1, 0x41
        CMP     r0, r1
        B       EQ, up

        ; Check Down (SDL 0x51, ASCII 's' 0x73, Alt 0x42)
        LI      r1, 0x51
        CMP     r0, r1
        B       EQ, down
        LI      r1, 0x73
        CMP     r0, r1
        B       EQ, down
        LI      r1, 0x42
        CMP     r0, r1
        B       EQ, down

        ; Check Left (SDL 0x50, ASCII 'a' 0x61, Alt 0x44)
        LI      r1, 0x50
        CMP     r0, r1
        B       EQ, left
        LI      r1, 0x61
        CMP     r0, r1
        B       EQ, left
        LI      r1, 0x44
        CMP     r0, r1
        B       EQ, left

        ; Check Right (SDL 0x4F, ASCII 'd' 0x64, Alt 0x43)
        LI      r1, 0x4F
        CMP     r0, r1
        B       EQ, right
        LI      r1, 0x64
        CMP     r0, r1
        B       EQ, right
        LI      r1, 0x43
        CMP     r0, r1
        B       EQ, right

        B       AL, isr_done

up:    LI      r5, -1
        B       AL, store
down:  LI      r5, 1
        B       AL, store
left:  LI      r4, -1
        B       AL, store
right: LI      r4, 1
store:
        ; Store new deltas and flag
        LI      r7, >next_dx
        LI      r6, <next_dx
        SB      r4, [a3]
        SB      r5, [a3+1]
        LI      r0, 1
        LI      r6, <moved
        SB      r0, [a3]

isr_done:
        ; Restore registers
        POP     r7
        POP     r6
        POP     r5
        POP     r4
        POP     r1
        POP     r0
        RETI

; --- Data Section ---
player_x_l: .db 0xB4
player_x_h: .db 0x00
player_y_l: .db 0xB4
player_y_h: .db 0x00
next_dx:  .db 0x00
next_dy:  .db 0x00
moved:    .db 0x00