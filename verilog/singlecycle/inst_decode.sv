module inst_decode(
	input  logic [15:0] 	i_instr,
	output logic [3:0] 	o_op1,
	output logic 		o_opi,
	output logic [2:0] 	o_op2,
	output logic [2:0] 	o_rsel2,
	output logic [2:0] 	o_rsel1,
	output logic 		o_opbr,
	output logic [1:0] 	o_asel,
	output logic [2:0] 	o_wsel
);

	assign o_op1 = i_instr[3:0];
	assign o_opi = i_instr[4];
	assign o_op2 = i_instr[6:4];
	assign o_rsel2 = i_instr[9:7];
	assign o_rsel1 = i_instr[12:10];
	assign o_opbr = i_instr[13];
	assign o_asel = i_instr[14:13];
	assign o_wsel = i_instr[15:13];

endmodule