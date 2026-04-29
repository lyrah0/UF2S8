init_stack:
        LI	r0, 0xFD
        MOV	sph, r0
        LI	r0, 0xFF
        MOV	spl, r0

init_io_addresses:
        LI	r7, 0xFE
        LI	r6, 0xF0

init_flags:
        LI      r0, 0x80
        MOV     flags, r0


draw_map:
        LI	r5, >clear_screen
        LI	r4, <clear_screen
draw_map_loop:
        LB	r0, [a2]
        B	ZS, draw_map_end
        SB	r0, [a3]
        ADD	r4, r4, 1
        INCC    r5
        B	AL, draw_map_loop
draw_map_end:
        MOV      r0, flags
        LI      r1, 0x80
        AND     r0, r0, r1
        CMP     r0, r1
        B       EQ, wait_for_interrupt
        RETI
wait_for_interrupt:
        B       AL, wait_for_interrupt

game_loop:
        LI      r5, >level_data
        LI      r4, <level_data
game_loop_find_player:
        LB      r2, [a2]
        LI      r1, 0x40        ; @
        CMP     r2, r1
        B	EQ, game_loop_found_player_at
        LI      r1, 0x2b        ; +
        CMP     r2, r1
        B	EQ, game_loop_found_player_plus
        ADD     r4, r4, 1
        INCC    r5
        B	AL, game_loop_find_player
game_loop_found_player_at:
        LI      r1, 0x20        ; space
        SB      r1, [a2]
        B	AL, game_loop_found_player
game_loop_found_player_plus:
        LI      r1, 0x2e        ; .
        SB      r1, [a2]
        B       AL, game_loop_found_player
game_loop_found_player:
game_loop_find_direction:
        LI      r1, 0x77        ; w
        CMP     r0, r1
        B       EQ, game_loop_w
        LI      r1, 0x61        ; a
        CMP     r0, r1
        B       EQ, game_loop_a
        LI      r1, 0x73        ; s
        CMP     r0, r1
        B       EQ, game_loop_s
        LI      r1, 0x64        ; d
        CMP     r0, r1
        B       EQ, game_loop_d

game_loop_w:
        ADD     r4, r4, -9
        DECC    r5
        LB      r3, [a2]
        LI      r1, 0x23        ; #
        CMP     r3, r1
        B       EQ, game_loop_w_wall
        LI      r1, 0x2e        ; .
        CMP     r3, r1
        B       EQ, game_loop_w_dot
        LI      r1, 0x24        ; $
        CMP     r3, r1
        B       EQ, game_loop_w_box
        B       AL, game_loop_w_space
game_loop_w_wall:
        ADD     r4, r4, 9
        INCC    r5
        LI      r1, 0x2e        ; .
        LB      r0, [a2]
        CMP     r0, r1
        B       EQ, game_loop_w_wall_dot
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map
game_loop_w_wall_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_w_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_w_box: ; TODO implement box pushing
        B       AL, draw_map
game_loop_w_space:
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map

game_loop_a:
        ADD     r4, r4, -1
        DECC    r5
        LB      r3, [a2]
        LI      r1, 0x23        ; #
        CMP     r3, r1
        B       EQ, game_loop_a_wall
        LI      r1, 0x2e        ; .
        CMP     r3, r1
        B       EQ, game_loop_a_dot
        LI      r1, 0x24        ; $
        CMP     r3, r1
        B       EQ, game_loop_a_box
        B       AL, game_loop_a_space
game_loop_a_wall:
        ADD     r4, r4, 1
        INCC    r5
        LI      r1, 0x2e        ; .
        LB      r0, [a2]
        CMP     r0, r1
        B       EQ, game_loop_a_wall_dot
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map
game_loop_a_wall_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_a_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_a_box: ; TODO implement box pushing
        B       AL, draw_map
game_loop_a_space:
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map

game_loop_s:
        ADD     r4, r4, 9
        INCC    r5
        LB      r3, [a2]
        LI      r1, 0x23        ; #
        CMP     r3, r1
        B       EQ, game_loop_s_wall
        LI      r1, 0x2e        ; .
        CMP     r3, r1
        B       EQ, game_loop_s_dot
        LI      r1, 0x24        ; $
        CMP     r3, r1
        B       EQ, game_loop_s_box
        B       AL, game_loop_s_space
game_loop_s_wall:
        ADD     r4, r4, -9
        DECC    r5
        LI      r1, 0x2e        ; .
        LB      r0, [a2]
        CMP     r0, r1
        B       EQ, game_loop_s_wall_dot
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map
game_loop_s_wall_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_s_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_s_box: ; TODO implement box pushing
        B       AL, draw_map
game_loop_s_space:
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map

game_loop_d:
        ADD     r4, r4, 1
        INCC    r5
        LB      r3, [a2]
        LI      r1, 0x23        ; #
        CMP     r3, r1
        B       EQ, game_loop_d_wall
        LI      r1, 0x2e        ; .
        CMP     r3, r1
        B       EQ, game_loop_d_dot
        LI      r1, 0x24        ; $
        CMP     r3, r1
        B       EQ, game_loop_d_box
        B       AL, game_loop_d_space
game_loop_d_wall:
        ADD     r4, r4, -1
        DECC    r5
        LI      r1, 0x2e        ; .
        LB      r0, [a2]
        CMP     r0, r1
        B       EQ, game_loop_d_wall_dot
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map
game_loop_d_wall_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_d_dot:
        LI      r1, 0x2b        ; +
        SB      r1, [a2]
        B       AL, draw_map
game_loop_d_box: ; TODO implement box pushing
        B       AL, draw_map
game_loop_d_space:
        LI      r1, 0x40        ; @
        SB      r1, [a2]
        B       AL, draw_map


clear_screen:
.ascii  "\033[2J\033[H"
level_data:
.ascii  "########\n"
.ascii  "#      #\n"
.ascii  "#  @   #\n"
.ascii  "#   $  #\n"
.ascii  "#      #\n"
.ascii  "#   .  #\n"
.ascii  "#      #\n"
.asciz  "########\n"

.origin 0xfe00
	RETI
kbd_isr:
	LB	r0, [a3+1]
        LI	r1, 0x77        ; w
        CMP	r0, r1
        B	EQ, goto_game_loop
        LI	r1, 0x61        ; a
        CMP	r0, r1
        B	EQ, goto_game_loop
        LI	r1, 0x73        ; s
        CMP	r0, r1
        B	EQ, goto_game_loop
        LI	r1, 0x64        ; d
        CMP	r0, r1
        B	EQ, goto_game_loop
	RETI
goto_game_loop:
        LI      r3, >game_loop
        LI      r2, <game_loop
        B       AL, [a1]

.origin 0xff00
interrupt_vectors:
.dh 0xfe00, 0xfe00, 0xfe00, 0xfe02