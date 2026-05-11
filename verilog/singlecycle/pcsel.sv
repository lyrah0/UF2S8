module pcsel(
	input logic [1:0] i_sel,
	input logic [14:0] i_pc,
	input logic [15:0] i_addr,
	input logic [7:0] i_data,
	output logic [15:0] o_pc
);

	always_comb begin
		case (i_sel)
			2'h0: o_pc = i_pc + 1;
			2'h1: o_pc = i_addr;
			2'h2: o_pc = {i_pc[14:7],i_data};
			2'h3: o_pc = {i_data,i_pc[6:0],1'b0};
			default: o_pc = 16'bZ;
		endcase
	end


endmodule