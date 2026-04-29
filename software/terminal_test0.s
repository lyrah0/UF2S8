setupstack:
	LI	r0, 0xFD
	MOV	sph, r0
	LI	r0, 0xFF
	MOV	SPL, r0

welcomeprint:
	LI	r0, 0
	LI	r1, 0x80
	LI	r5, 0xFE
	LI	r4, 0xF0
	LI	r7, >welcome_message
	LI	r6, <welcome_message
	MOV	flags, r1
wprintloop:
	LB	r1, [a3+1]
	B	ZS, wprintend
	ADD	r6, r6, 1
	ADC	r7, r7, r0
	SB	r1, [a2]
	B	AL, wprintloop
wprintend:
	B	AL, wprintend


welcome_message:
.ascii	"\033[2J"
.ascii	"\033[H"
.ascii	"Welcome to the UF2S8v0 terminal! (v0)"
.ascii "\033[3;1H"
.db	0x00

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
