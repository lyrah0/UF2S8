module csr_file(
	input logic             i_clk,
	input logic             i_rst_n,
	input logic             i_we,
	input logic [2:0]       i_rsel,
	input logic [2:0]       i_wsel,
	input logic [7:0]       i_wdata,
	input logic [15:0]      i_sp,
	input logic             i_wsp,
	output logic [7:0]      o_rdata,
	output logic [15:0]     o_sp
);

	logic [7:0] r_reg [7:0];

	assign o_rdata = r_reg[i_rsel];
	assign o_sp = {r_reg[7],r_reg[6]};

	always_ff @(posedge i_clk) begin
		if (!i_rst_n) begin
			for (byte i = 0; i < 8; i++) r_reg[i[2:0]] <= 8'b0;
		end
		if (i_wsel < 6 && i_wsel > 0) begin
		end else if (i_we) begin
			r_reg[i_wsel] <= i_wdata;
		end
		if (i_wsp) begin
			r_reg[6] <= i_sp[7:0];
			r_reg[7] <= i_sp[15:8];
		end
	end
	

endmodule