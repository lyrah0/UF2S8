module control_unit(
	// Clock and Reset
	input logic		i_clk,
	input logic		i_rst_n,

	// Decoder inputs
	input  logic [15:0]	i_instr,
	input  logic [3:0]	i_op1,
	input  logic 		i_opi,
	input  logic [2:0]	i_op2,
	input  logic [2:0]	i_rsel2,
	input  logic [2:0]	i_rsel1,
	input  logic 		i_opbr,
	input  logic [1:0]	i_asel,
	input  logic [2:0]	i_wsel,

	// ALU
	output logic [3:0]	o_alu_op,
	
	// Register File
	output logic		o_rf_we,
	output logic [2:0]	o_rf_rsel1,
	output logic [2:0]	o_rf_rsel2,
	
	// CSR File
	output logic 		o_cf_we,
	output logic [2:0]	o_cf_rsel,
	output logic [2:0]	o_cf_wsel,
	output logic 		o_cf_wsp,
	
	// immediate generator
	output logic [2:0]	o_ig_sel,

	// Address generation unit
	output logic [2:0]	o_agu_sel,
	
	// stack pointer selector
	output logic [1:0]	o_sp_sel,
	
	// PC selector
	output logic [1:0]	o_pc_sel
);


endmodule