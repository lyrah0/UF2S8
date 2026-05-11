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

	// instruction stage flip flops
	logic		r_stage0;
	logic		r_stage1;
	logic		r_stage2;
	logic		r_stage3;


	always_comb begin
		
	end

	always_ff @(posedge i_clk or negedge i_rst_n) begin
		if (!i_rst_n) begin
			r_stage0 <= 1'b1;
			r_stage1 <= 1'b0;
			r_stage2 <= 1'b0;
			r_stage3 <= 1'b0;
		end else begin
			r_stage0 <= 1'b1;
			r_stage1 <= r_stage0;
			r_stage2 <= r_stage1;
			r_stage3 <= r_stage2;
		end
	end

endmodule