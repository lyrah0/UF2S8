module inst_decode(
	input  logic [15:0] 	i_instr,
	output logic [2:0] 	o_rsel2,
	output logic [2:0] 	o_rsel1,
	output logic [2:0] 	o_wsel
);

	assign o_rsel2 = i_instr[9:7];
	assign o_rsel1 = i_instr[12:10];
	assign o_wsel = i_instr[15:13];

endmodule