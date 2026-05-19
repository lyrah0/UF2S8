`timescale 1ns / 1ps
module register_file(
	input  logic        i_clk,
	input  logic        i_we,
	input  logic [2:0]  i_rsel1,
	input  logic [2:0]  i_rsel2,
	input  logic [2:0]  i_wsel,
	input  logic [7:0]  i_wdata,
	output logic [7:0]  o_rdata1,
	output logic [7:0]  o_rdata2,
	output logic [15:0] o_adata
);
	logic [7:0] r_gpr [0:7];


	assign o_rdata1 = r_gpr[i_rsel1];
	assign o_rdata2 = r_gpr[i_rsel2];
	assign o_adata  = {r_gpr[{i_rsel2[2:1],1'b1}],
			   r_gpr[{i_rsel2[2:1],1'b0}]};

	always_ff @(posedge i_clk) begin
		if (i_we) begin
			r_gpr[i_wsel] <= i_wdata;
		end
	end


endmodule