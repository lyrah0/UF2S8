`timescale 1ns/1ps

module stage_fetch (
	input logic		i_clk,
	input logic		i_rst,

	input logic		i_stall,
	input logic		i_pc_lwe,
	input logic		i_pc_uwe,
	input logic		i_pc_we,
	input logic [7:0]	i_data,
	input logic [15:1]	i_adata,
	output logic [15:1]	o_pc,
	output logic [15:1]	o_npc,
	output logic [15:0]	o_inst,

	// Memory interface
	input  logic		imem_stall_i,
	input  logic [15:0]	imem_dat_i,
	output logic [15:1]	imem_adr_o
);

	logic [15:1]	r_pc;

	always_comb begin
		o_pc = r_pc;
		o_npc = r_pc + 1;
		imem_adr_o = r_pc;
		if (i_stall || imem_stall_i)
			o_inst = 16'd0;
		else
			o_inst = imem_dat_i;
	end

	always_ff @(posedge i_clk) begin
		if (i_rst) begin
			r_pc <= 15'd0;
		end else if (i_pc_lwe) begin
			r_pc[7:1] <= i_data[7:1];
		end else if (i_pc_uwe) begin
			r_pc[15:8] <= i_data;
		end else if (i_pc_we) begin
			r_pc <= i_adata;
		end else if (~(i_stall || imem_stall_i)) begin
			r_pc <= o_npc;
		end
	end

endmodule