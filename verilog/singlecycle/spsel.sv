module spsel(
	input logic [1:0]	i_sel,
	input logic [15:0]	i_sp,
	output logic [15:0]	o_sp
);

	always_comb begin
		case (i_sel)
			2'h0: o_sp = i_sp;
			2'h1: o_sp = i_sp + 1;
			2'h2: o_sp = i_sp - 1;
			default: o_sp = 16'bZ;
		endcase
	end


endmodule