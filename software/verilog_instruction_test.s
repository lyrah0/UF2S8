start:
	NOP
init_stack:
	LI	r0, 0xFD
	MOV	sph, r0
	LI	r0, 0xFF
	MOV	spl, r0

test_stack:
	LI	r0, 0xAB
	LI	r1, 0xF0
	PUSH	r0
	PUSH	r1
	POP	r2
	POP	r3
test_link_and_return:
	LI	r0, 0x1
	LI	r1, 0x1
	CMP	r0, r1
	BL	EQ, return_test
	LI	r7, >return_test
	LI	r6, <return_test
	LI	r5, 0xFF
	ADD	r5, r5, 5
	BL	CS, a3
test_move:
	MOV	r5, r2
	MOV	r0, r3
	LI	r0, 0
	MOV	r0, flags
	MOV	r2, spl
	MOV	r3, sph
test_arithmetic:
	LI	r0, 0x10
	LI	r1, 0x01
	LI	r2, 0xFF
	LI	r3, 0x56
	SUB	r4, r0, r1
	SUB	r4, r0, r2
	SBB	r5, r3, r1
	ADD	r4, r2, r1
	ADC	r5, r3, r0
	SUB	r4, r1, r0
	DECB	r5
	ADD	r4, r2, r1
	INCC	r4
test_logic:
	LI	r0, 0xFF
	LI	r1, 0xAB
	LI	r2, 0x0F
	AND	r3, r0, r1
	OR	r3, r1, r2
	NOR	r3, r1, r2
	XOR	r3, r0, r1
test_shift:
	LI	r0, 0xAB
	LI	r1, 0x55
	LI	r2, 5
	LI	r3, 7
	LI	r4, 2
	SLL	r5, r1, r2
	SRL	r5, r0, r3
	SRA	r5, r0, r4

	SLL	r6, r1, 1
	SRL	r6, r1, 1
	SRA	r6, r0, 4
test_branch:
	LI	r0, -1
	B	NS, test_branch_a
test_branch_fail1:
	B	AL, test_branch_fail1
test_branch_a:
	LI	r7, >test_loadstore
	LI	r6, <test_loadstore
	LI	r0, 0x55
	LI	r1, 0xAA
	CMA	r0, r1
	B	ZS, a3
test_branch_fail2:
	B	AL, test_branch_fail2
test_loadstore:
	LI	r7, 0xF0
	LI	r6, 0x00
	LI	r0, 0x12
	LI	r1, 0xAB
	SB	r0, [a3]
	SB	r0, [a3+15]
	LB	r2, [a3]
	LB	r3, [a3+15]
test_interrupt:
	LI	r7, 0xFF
	LI	r6, 0x00
	LI	r0, <int_handler
	LI	r1, >int_handler
	SB	r0, [a3]
	SB	r1, [a3+1]
	LI	r0, 0
	SWI	r0
	WFI
halt:
	B	AL, halt

return_test:
	RET



int_handler:
	RETI