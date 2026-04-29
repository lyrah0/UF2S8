init_stack:
        LI	r0, 0xFD
        MOV	sph, r0
        LI	r0, 0xFF
        MOV	spl, r0
init_interrupts:
        LI	r7, >kbd_isr
        LI	r6, <kbd_isr
        LI      r5, 0xFF
        LI      r4, 0x06
        SB      r6, [a2]
        ADD     r4, r4, 1
        SB      r7, [a2]        
init_flags:
        LI      r0, 0x80
        MOV     flags, r0



draw_map:
        MOV     r0, flags
        PUSH    r0
        LI      r0, 0x00
        MOV     flags, r0
        LI	r5, >clear_screen
        LI	r4, <clear_screen
init_io_addresses:
        LI	r7, 0xFE
        LI	r6, 0xF0
draw_map_loop:
        LB	r2, [a2]
        B	ZS, draw_map_end
        SB	r2, [a3]
        ADD	r4, r4, 1
        INCC    r5
        B	AL, draw_map_loop
draw_map_end:
        POP     r0
        MOV     flags, r0
        ; Check if we should RETI (inside interrupt) or Wait (main thread)
        MOV      r2, flags
        LI      r3, 0x80
        AND     r2, r2, r3      ; If flags bit 7 is 1, we are in main thread
        CMP     r2, r3
        B       EQ, wait_for_interrupt
        RETI
wait_for_interrupt:
        B       AL, wait_for_interrupt


game_loop:
        ; Save move vector (r5:4) to r3:2 (a1) so draw_map doesn't erase it
        MOV     r3, r5
        MOV     r2, r4
        LI      r7, >level_data
        LI      r6, <level_data
game_loop_find_player:
        LB      r0, [a3]
        LI      r1, 0x40        ; @
        CMP     r0, r1
        B	EQ, game_loop_found_player_at
        LI      r1, 0x2b        ; +
        CMP     r0, r1
        B	EQ, game_loop_found_player_plus
        ADD     r6, r6, 1
        INCC    r7
        B	AL, game_loop_find_player
game_loop_found_player_at:
        LI      r1, 0x20        ; space
        SB      r1, [a3]
        B	AL, game_loop_move
game_loop_found_player_plus:
        LI      r1, 0x2e        ; .
        SB      r1, [a3]
        B       AL, game_loop_move

game_loop_move:
        ADD     r6, r6, r2
        ADC     r7, r7, r3
        LB      r0, [a3]
        LI      r1, 0x23        ; #
        CMP     r0, r1
        B       EQ, game_loop_move_wall
        LI      r1, 0x2e        ; .
        CMP     r0, r1
        B       EQ, game_loop_move_dot
        LI      r1, 0x24        ; $
        CMP     r0, r1
        B       EQ, game_loop_move_box
        LI      r1, 0x2a        ; *
        CMP     r0, r1
        B       EQ, game_loop_move_box_on_dot
        B       AL, game_loop_move_space
game_loop_move_wall:
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x2e        ; .
        LB      r0, [a3]
        CMP     r0, r1
        B       EQ, game_loop_move_wall_dot
        LI      r1, 0x40        ; @
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_wall_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_box:
        ADD     r6, r6, r2
        ADC     r7, r7, r3
        LI      r1, 0x20        ; space
        LB      r0, [a3]
        CMP     r0, r1
        B       EQ, game_loop_move_box_space
        LI      r1, 0x2e        ; .
        CMP     r0, r1
        B       EQ, game_loop_move_box_dot
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x40        ; @
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_box_space:
        LI      r1, 0x24        ; $
        SB      r1, [a3]
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x40        ; @
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_box_dot:
        LI      r1, 0x2a        ; *
        SB      r1, [a3]
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x40        ; @
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_box_on_dot:
        ; a3 is '*' box. Move forward to check behind it
        ADD     r6, r6, r2
        ADC     r7, r7, r3
        LB      r0, [a3]
        LI      r1, 0x20        ; space
        CMP     r0, r1
        B       EQ, game_loop_move_box_on_dot_space
        LI      r1, 0x2e        ; .
        CMP     r0, r1
        B       EQ, game_loop_move_box_on_dot_dot
        ; Blocked: move back twice to original player spot
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x2b        ; + (restore player on dot)
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_box_on_dot_space:
        LI      r1, 0x24        ; $
        SB      r1, [a3]
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x2b        ; +
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_box_on_dot_dot:
        LI      r1, 0x2a        ; *
        SB      r1, [a3]
        SUB     r6, r6, r2
        SBC     r7, r7, r3
        LI      r1, 0x2b        ; +
        SB      r1, [a3]
        B       AL, draw_map
game_loop_move_space:
        LI      r1, 0x40        ; @
        SB      r1, [a3]
        B       AL, draw_map



clear_screen:
.ascii  "\033[2J\033[H"
level_data:
.ascii  "########\n"
.ascii  "#      #\n"
.ascii  "# @    #\n"
.ascii  "#   $  #\n"
.ascii  "#      #\n"
.ascii  "#   .  #\n"
.ascii  "#      #\n"
.asciz  "########\n"



kbd_isr:
	LB	r0, [a3+1]
        LI	r1, 0x77        ; w
        CMP	r0, r1
        B	EQ, pressed_w
        LI	r1, 0x61        ; a
        CMP	r0, r1
        B	EQ, pressed_a
        LI	r1, 0x73        ; s
        CMP	r0, r1
        B	EQ, pressed_s
        LI	r1, 0x64        ; d
        CMP	r0, r1
        B	EQ, pressed_d
        LI	r1, 0x41        ; Arrow Up
        CMP	r0, r1
        B	EQ, pressed_w
        LI	r1, 0x42        ; Arrow Down
        CMP	r0, r1
        B	EQ, pressed_s
        LI	r1, 0x43        ; Arrow Right
        CMP	r0, r1
        B	EQ, pressed_d
        LI	r1, 0x44        ; Arrow Left
        CMP	r0, r1
        B	EQ, pressed_a
	RETI

pressed_w:
        LI      r4, -9
        LI      r5, 0xff
        B       AL, goto_game_loop
pressed_a:
        LI      r4, -1
        LI      r5, 0xff
        B       AL, goto_game_loop
pressed_s:
        LI      r4, 9
        LI      r5, 0
        B       AL, goto_game_loop
pressed_d:
        LI      r4, 1
        LI      r5, 0
        B       AL, goto_game_loop

goto_game_loop:
        LI      r3, >game_loop
        LI      r2, <game_loop
        B       AL, [a1]