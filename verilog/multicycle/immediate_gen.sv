`timescale 1ns / 1ps
module immediate_gen(
	input  logic [15:4]	i_instr,
	input  logic [2:0] 	i_sel,
	output logic [15:0]	o_immediate
);

	always_comb begin
		case (i_sel)
			3'h0: o_immediate = {{10{i_instr[6]}}, i_instr[6:4], i_instr[9:7]};
			3'h1: o_immediate = {{8{i_instr[12]}}, i_instr[12:5]};
			3'h2: o_immediate = {{9{i_instr[15]}}, i_instr[15:13], i_instr[7:4]};
			3'h3: o_immediate = {{9{i_instr[12]}}, i_instr[12:10], i_instr[7:4]};
			3'h4: o_immediate = {{6{i_instr[12]}}, i_instr[12:4], 1'b0};
			3'h5: o_immediate = 16'h0;
			3'h6: o_immediate = 16'h1;
			3'h7: o_immediate = 16'h2;
			default: o_immediate = 16'bx;
		endcase
	end


endmodule