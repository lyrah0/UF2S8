`timescale 1ns / 1ps

module ram (
	input  logic		i_clk,

	// Instruction port
	input  logic [15:1]	i_imem_adr,
	output logic [15:0]	o_imem_dat,
	output logic		o_imem_stall,

	// Data port
	input  logic [15:0]	i_dmem_adr,
	input  logic [7:0]	i_dmem_dat,
	input  logic		i_dmem_re,
	input  logic		i_dmem_we,
	output logic [7:0]	o_dmem_dat,
	output logic		o_dmem_stall
);

	logic [7:0] mem [0:65535];

	// Instruction read (16-bit word from 15-bit word address)
	assign o_imem_dat   = {mem[{i_imem_adr, 1'b1}], mem[{i_imem_adr, 1'b0}]};
	assign o_imem_stall = 1'b0;

	// Data read (8-bit byte from 16-bit byte address)
	assign o_dmem_dat   = i_dmem_re ? mem[i_dmem_adr] : 8'bx;
	assign o_dmem_stall = 1'b0;

	// Synchronous write
	always_ff @(posedge i_clk) begin
		if (i_dmem_we) begin
			mem[i_dmem_adr] <= i_dmem_dat;
		end
	end

endmodule
