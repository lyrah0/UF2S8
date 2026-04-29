; Bouncing Square Graphics Test for uf2s8
; Resolution: 128x128
; VRAM: 0x8000 - 0xBFFF

.origin 0x0000

init:
    ; Parameters
    LI   r0, 60             ; r0: current x
    LI   r1, 30             ; r1: current y
    LI   r2, 1              ; r2: dx (velocity x)
    LI   r3, 1              ; r3: dy (velocity y)
    LI   r4, 0x1C           ; r4: color (Greenish-Blue in RGB332)

main_loop:
    ; 1. Draw 4x4 Square
    PUSH r0
    PUSH r1
    PUSH r4
    BL   AL, draw_square
    POP  r4
    POP  r1
    POP  r0

    ; 2. Delay (approx 16ms to target ~60fps)
    LI   r5, 0x40           ; Adjust delay
delay_outer:
    LI   r6, 0xFF
delay_inner:
    ADD  r6, r6, -1
    B    NE, delay_inner
    ADD  r5, r5, -1
    B    NE, delay_outer

    ; 3. Erase previous square
    PUSH r0
    PUSH r1
    PUSH r4
    LI   r4, 0x00           ; Black color
    BL   AL, draw_square
    POP  r4
    POP  r1
    POP  r0

    ; 4. Update Position
    ADD  r0, r0, r2
    ADD  r1, r1, r3

    ; 5. Collision Check X (0 to 124 for 4x4 square)
    LI   r5, 0
    CMP  r0, r5
    B    EQ, bounce_x
    LI   r5, 124
    CMP  r0, r5
    B    EQ, bounce_x
    B    AL, check_y

bounce_x:
    LI   r5, 0
    SUB  r2, r5, r2         ; dx = -dx
    LI   r5, 0x25
    ADD  r4, r4, r5         ; Change color

check_y:
    ; 6. Collision Check Y (0 to 124)
    LI   r5, 0
    CMP  r1, r5
    B    EQ, bounce_y
    LI   r5, 124
    CMP  r1, r5
    B    EQ, bounce_y
    B    AL, main_loop

bounce_y:
    LI   r5, 0
    SUB  r3, r5, r3         ; dy = -dy
    LI   r5, 0x42
    ADD  r4, r4, r5         ; Change color
    B    AL, main_loop

; --- Helper: Draw 4x4 Square ---
; Inputs: r0=x, r1=y, r4=color
draw_square:
    LI   r5, 4              ; Row counter
draw_row:
    PUSH r5
    PUSH r0
    PUSH r1

    ; Address calculation for row: 0x8000 + (y << 7) + x
    ; High Byte (r7): 0x80 + (y >> 1)
    MOV  r7, r1
    SRL  r7, r7, 1
    LI   r5, 0x80
    ADD  r7, r7, r5
    
    ; Low Byte (r6): ((y & 1) << 7) + x
    MOV  r6, r1
    LI   r5, 1
    AND  r6, r6, r5
    SLL  r6, r6, 7
    ADD  r6, r6, r0

    ; Draw 4 pixels (using a3 as r6:r7 pair)
    SB   r4, [a3+0]
    SB   r4, [a3+1]
    SB   r4, [a3+2]
    SB   r4, [a3+3]

    POP  r1
    POP  r0
    POP  r5
    
    ADD  r1, r1, 1          ; Move to next y
    ADD  r5, r5, -1
    B    NE, draw_row
    RET
