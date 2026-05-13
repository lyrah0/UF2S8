module pcsel(
	input logic		i_clk,
	input logic [1:0]	i_sel,
	input logic [14:0]	i_pc,
	input logic [15:0]	i_offset,
	input logic [15:0]	i_addr,
	input logic [7:0]	i_data,
	input logic		i_we,
	output logic [15:0]	o_pc
);

	logic [6:0]	r_pc; 

	always_comb begin
		case (i_sel)
			2'h0: o_pc = {i_pc + 1'b1,1'b0};
			2'h1: o_pc = i_addr;
			2'h2: o_pc = {i_pc + 1'b1,1'b0} + i_offset;
			2'h3: o_pc = {i_data,r_pc,1'b0};
			default: o_pc = 16'bZ;
		endcase
	end

	always_ff @(posedge i_clk) begin
		if (i_we) begin
			r_pc <= i_data[7:1];
		end
	end


endmodule