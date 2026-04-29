.equ STACK_TOP, 0xFDFF
.equ VRAM_START, 0xC000
.equ VRAM_SIZE, 0x2000
.equ KBD_INT_VECTOR, 0xFF06
.equ KBD_STATUS, 0xFEF2
.equ KBD_DATA, 0xFEF1
.equ HW_CONTROL, 0xFEFF
.equ GRAPHICS_CONTROL, 0xFEFD
.equ INITIAL_POS, 0x80

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
        ; Draw initial dot at stored position
        LI      r7, >player_x
        LI      r6, <player_x
        LB      r2, [a3]        ; x
        LB      r3, [a3+1]      ; y
        LI      r0, 1           ; color=set
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
        LI      r7, >player_x
        LI      r6, <player_x
        LB      r2, [a3]        ; x
        LB      r3, [a3+1]      ; y
        LI      r0, 0           ; color=clear
        BL      AL, draw_pixel

        ; Update X: x += dx  (wraps 0-255 naturally)
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
        SB      r2, [a3]        ; store new x
        SB      r3, [a3+1]      ; store new y

        ; Draw new pixel
        LI      r0, 1           ; color=set
        BL      AL, draw_pixel
        B       AL, wait_for_input


; --- draw_pixel routine ---
; Inputs: r2 = x, r3 = y, r0 = color (0 or 1)
; Clobbers: r1, r4, r5, r6, r7
draw_pixel:
        PUSH    r0              ; save color

        ; Address = 0xC000 + (y * 32) + (x / 8)
        ; y * 32 = y << 5: low byte = y << 5, high = y >> 3
        MOV     r4, r3
        SLL     r4, r4, 5       ; r4 = low byte of y*32
        MOV     r5, r3
        SRL     r5, r5, 3       ; r5 = high byte of y*32

        ; x / 8
        MOV     r1, r2
        SRL     r1, r1, 3       ; r1 = x/8

        ; Add x/8 to low byte, carry into high, add base 0xC0
        ADD     r4, r4, r1
        LI      r1, 0
        ADC     r5, r5, r1
        LI      r1, 0xC0
        ADD     r5, r5, r1      ; r5:r4 = VRAM address (a2)

        ; Mask = 1 << (7 - (x % 8))
        LI      r1, 7
        MOV     r6, r2
        LI      r7, 0x07
        AND     r6, r6, r7      ; x % 8
        SUB     r6, r1, r6      ; 7 - (x % 8)
        LI      r7, 1
        SLL     r7, r7, r6      ; r7 = bit mask

        ; Read-Modify-Write
        LB      r1, [a2]        ; load VRAM byte

        POP     r0              ; restore color
        CMA     r0, r0
        B       EQ, dp_clear
dp_set:
        OR      r1, r1, r7
        B       AL, dp_store
dp_clear:
        NOR     r7, r7, r7      ; invert mask
        AND     r1, r1, r7
dp_store:
        SB      r1, [a2]
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
player_x: .db 0x80
player_y: .db 0x80
next_dx:  .db 0x00
next_dy:  .db 0x00
moved:    .db 0x00