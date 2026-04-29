; Dynamic Bouncing Square Graphics Test for uf2s8
; Resolution: 128x128
; VRAM: 0x8000 - 0xBFFF
; This version changes the bounce angle on every collision.

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

    ; 2. Delay
    PUSH r7
    LI  r7, 0x40
delay_outerer:
    LI   r5, 0xFF
delay_outer:
    LI   r6, 0xFF
delay_inner:
    ADD  r6, r6, -1
    B    NE, delay_inner
    ADD  r5, r5, -1
    B    NE, delay_outer
    ADD  r7, r7, -1
    B    NE, delay_outerer
    POP  r7

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

    ; 5. Collision Check X (Right Wall: >= 124, Left Wall: < 0 [>=128 unsigned])
    LI   r5, 124
    CMP  r0, r5
    B    CC, check_y        ; x < 124, check Y

    ; We hit an X wall! Determine if it was Left or Right for clamping
    LI   r5, 0x80
    CMP  r0, r5
    B    CS, hit_left
hit_right:
    LI   r0, 124            ; Clamp to right edge
    B    AL, bounce_x
hit_left:
    LI   r0, 0              ; Clamp to left edge

bounce_x:
    LI   r5, 0              ; Flip dx
    SUB  r2, r5, r2
    
    ; Change Angle: Modify dy magnitude (cycle 1-3)
    LI   r5, 0
    CMP  r3, r5
    B    NS, dy_neg
dy_pos:
    ADD  r3, r3, 1          ; 1->2, 2->3, 3->4
    LI   r5, 4
    CMP  r3, r5
    B    NE, bounce_x_done
    LI   r3, 1              ; Wrap to 1
    B    AL, bounce_x_done
dy_neg:
    ADD  r3, r3, -1         ; -1->-2, -2->-3, -3->-4
    LI   r5, -4
    CMP  r3, r5
    B    NE, bounce_x_done
    LI   r3, -1             ; Wrap to -1
bounce_x_done:
    LI   r5, 0x25
    ADD  r4, r4, r5         ; Change color
    ; Fall through to check_y

check_y:
    ; 6. Collision Check Y (Bottom: >= 124, Top: < 0 [>= 128 unsigned])
    LI   r5, 124
    CMP  r1, r5
    B    CC, jump_back      ; y < 124, jump to loop

    ; We hit a Y wall! Determine if it was Top or Bottom for clamping
    LI   r5, 0x80
    CMP  r1, r5
    B    CS, hit_top
hit_bottom:
    LI   r1, 124            ; Clamp to bottom edge
    B    AL, bounce_y
hit_top:
    LI   r1, 0              ; Clamp to top edge

bounce_y:
    LI   r5, 0              ; Flip dy
    SUB  r3, r5, r3
    
    ; Change Angle: Modify dx magnitude (cycle 1-3)
    LI   r5, 0
    CMP  r2, r5
    B    NS, dx_neg
dx_pos:
    ADD  r2, r2, 1
    LI   r5, 4
    CMP  r2, r5
    B    NE, bounce_y_done
    LI   r2, 1
    B    AL, bounce_y_done
dx_neg:
    ADD  r2, r2, -1
    LI   r5, -4
    CMP  r2, r5
    B    NE, bounce_y_done
    LI   r2, -1
bounce_y_done:
    LI   r5, 0x42
    ADD  r4, r4, r5         ; Change color

jump_back:
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
