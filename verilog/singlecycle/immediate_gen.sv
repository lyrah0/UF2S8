module immediate_gen(
	input  logic [15:0]	i_instr,
	input  logic [2:0] 	i_sel,
	output logic [15:0]	o_immediate
);

	always_comb begin
		case (i_sel)
			3'h0: o_immediate = {{13{i_instr[9]}}, i_instr[9:7]};
			3'h1: o_immediate = {{8{i_instr[12]}},i_instr[12:5]};
			3'h2: o_immediate = {{10{i_instr[9]}},i_instr[9:4]};
			3'h3: o_immediate = {{9{i_instr[10]}},i_instr[10:4]};
			3'h4: o_immediate = {{6{i_instr[12]}},i_instr[12:4],1'b0};
			default: o_immediate = 16'b0;
		endcase
	end


endmodule