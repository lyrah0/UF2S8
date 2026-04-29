setupstack:
	LI	r0, 0xFD
	MOV	sph, r0
	LI	r0, 0xFF
	MOV	spl, r0

welcomeprint:
	LI	r0, 0x80
	MOV	flags, r0
	LI	r5, 0xFE
	LI	r4, 0xF0
	LI	r7, >welcome_message
	LI	r6, <welcome_message
wprintloop:
	LB	r0, [a3]
	B	ZS, wprintend
	ADD	r6, r6, 1
	INCC	r7
	SB	r0, [a2]
	B	AL, wprintloop
wprintend:
	LI	r0, 0
;	SWI	r0
	B	AL, wprintend


welcome_message:
.asciz	"\033[2J\033[HWelcome to the UF2S8 terminal! (v0)\033[3;1H"

.origin 0xfe00
	RETI
kbd_isr:
	PUSH	r7
	PUSH	r6
	PUSH	r5
	LI	r7, 0xFE
	LI	r6, 0xF0
	LB	r5, [a3]
	SB	r5, [a3+1]
	POP	r5
	POP	r6
	POP	r7
	RETI


.origin 0xff00
interrupt_vectors:
.dh 0xfe00, 0xfe00, 0xfe00, 0xfe02
