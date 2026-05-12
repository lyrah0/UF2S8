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
	output logic [1:0]	o_pc_sel,
	output logic		o_pc_rwe,

	// top level signals
	output logic		o_pc_we,
	output logic		o_alu_b_sel,
	output logic [1:0]	o_rf_wdata_sel,
	output logic [1:0]	o_mem_wdata_sel,
	output logic		o_mem_we,
	output logic		o_cf_wdata_sel
);

	// instruction stage flip flops
	logic [1:0]	r_stage;


	always_comb begin
		o_alu_op = 4'b0;
		o_rf_we = 1'b0;
		o_rf_rsel1 = i_rsel1;
		o_rf_rsel2 = i_rsel2;
		o_cf_we = 1'b0;
		o_cf_rsel = i_rsel1;
		o_cf_wsel = i_wsel;
		o_cf_wsp = 1'b0;
		o_ig_sel = 3'b0;
		o_agu_sel = 3'b0;
		o_sp_sel = 2'b0;
		o_pc_sel = 2'b0;
		o_pc_we = 1'b1;
		casez (i_instr)
			16'b000_000_000_000_0000: ; // NOP
			16'b001_000_000_000_0000: begin // RET
				o_sp_sel = 2'h1;

				if (r_stage == 0) begin
					o_pc_sel = 2'h2;
				end
				if (r_stage == 1) begin
					o_pc_sel = 2'h3;
				end
			end
			default: begin // no instruction
			end
		endcase
	end

	always_ff @(posedge i_clk or negedge i_rst_n) begin
		if (!i_rst_n) begin
			r_stage <= 2'b00;
		end

		casez (i_instr)
			16'b001_000_000_000_0000: if (r_stage == 0) begin
				r_stage <= r_stage + 1;
			end
			default: begin
				r_stage <= 2'b00;
			end		
		endcase

	end

endmodule