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

	// Flags
	input  logic [7:0]	i_flags,

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
	output logic [1:0]	o_cf_wdata_sel
);

	// instruction stage flip flops
	logic [1:0]	r_stage;

	logic 		w_cond;


	always_comb begin
		o_alu_op = 4'b0;
		o_rf_we = 1'b0;
		o_rf_rsel1 = i_rsel1;
		o_rf_rsel2 = i_rsel2;
		o_cf_we = 1'b0;
		o_cf_rsel = i_rsel1;
		o_cf_wsel = 3'h0;
		o_cf_wsp = 1'b0;
		o_ig_sel = 3'h0;
		o_agu_sel = 3'h0;
		o_sp_sel = 2'h0;
		o_pc_sel = 2'h0;
		o_pc_rwe = 1'b0;
		o_pc_we = 1'b1;
		o_alu_b_sel = 1'b0;
		o_rf_wdata_sel = 2'h0;
		o_mem_wdata_sel = 2'h0;
		o_mem_we = 1'b0;
		o_cf_wdata_sel = 2'h0;
		
		case (i_wsel)
			3'b000: w_cond = i_flags[1];	// ZS/EQ
			3'b001: w_cond = ~i_flags[1];	// ZC/NE
			3'b010: w_cond = i_flags[0];	// CS/HS
			3'b011: w_cond = ~i_flags[0];	// CC/LO
			3'b100: w_cond = i_flags[2];	// NS/MI
			3'b101: w_cond = ~i_flags[2];	// NC/PL
			3'b110: w_cond = i_flags[3];	// VS
			3'b111: w_cond = 1'b1;		// AL
		endcase

		casez (i_instr)
			16'b000_000_000_000_0000: ; // NOP
			16'b001_000_000_000_0000: begin // RET
				o_sp_sel = 2'h1;

				if (r_stage == 0) begin
					o_pc_rwe = 1'b1;
				end
				if (r_stage == 1) begin
					o_pc_sel = 2'h3;
				end
			end
			16'b???_010_000_000_0000: begin // INCC
				o_alu_op = 4'h1;
				o_rf_we = 1'b1;
				o_rf_rsel1 = i_wsel;
				o_cf_we = 1'b1;
				o_ig_sel = 3'h7;
				o_alu_b_sel = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_011_000_000_0000: begin // DECB
				o_alu_op = 4'h3;
				o_rf_we = 1'b1;
				o_rf_rsel1 = i_wsel;
				o_cf_we = 1'b1;
				o_ig_sel = 3'h7;
				o_alu_b_sel = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_110_000_000_0000: begin // POP
				o_rf_we = 1'b1;
				o_cf_wsp = 1'b1;
				o_cf_wdata_sel = 2'h2;
				o_sp_sel = 2'h1; 
				o_agu_sel = 3'h4;
				o_rf_wdata_sel = 2'h2;
			end
			16'b???_111_000_000_0000: begin // PUSH
				o_rf_rsel1 = i_wsel;
				o_cf_wsp = 1'b1;
				o_cf_wdata_sel = 2'h2;
				o_sp_sel = 2'h2; 
				o_agu_sel = 3'h3;
				o_mem_we = 1'b1;
			end
			16'b???_???_000_001_0000: begin // MOV csr->gpr
				o_rf_we = 1'b1;
				o_rf_wdata_sel = 2'h1;

				// handle flags
				o_cf_wdata_sel = 2'h2;
				o_cf_we = 1'b1;
			end
			16'b???_???_001_001_0000: begin // MOV gpr->csr
				o_cf_we = 1'b1;
				o_cf_wsel = i_wsel;
			end
			16'b???_???_010_001_0000: begin // CMP
				// operands
				o_rf_rsel2 = i_wsel;
				// flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h2;
			end
			16'b???_???_011_001_0000: begin // CMA
				// operands
				o_rf_rsel2 = i_wsel;
				// ALU op
				o_alu_op = 4'h4;
				// flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h2;
			end
			16'b???_??0_000_111_0000: begin // B
				if (w_cond) begin
					// load new PC
					o_rf_rsel2 = i_rsel1;
					o_agu_sel = 3'h0;
					o_pc_sel = 2'h1;
					o_pc_we = 1'b1;
				end
			end
			16'b???_??1_000_111_0000: begin // BL
				if (w_cond && r_stage == 0) begin
					// store PC upper
					o_agu_sel = 3'h3;
					o_mem_wdata_sel = 2'h3;
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					o_sp_sel = 2'h2;
				end
				if (w_cond && r_stage == 1) begin
					// store PC lower
					o_agu_sel = 3'h3;
					o_mem_wdata_sel = 2'h2;
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					o_sp_sel = 2'h2;
					// load new PC
					o_rf_rsel2 = i_rsel1;
					o_pc_sel = 2'h2;
					o_pc_we = 1'b1;
				end
			end
			16'b???_???_???_000_0001: begin // SUB
				// subtract
				o_alu_op = 4'h2;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_001_0001: begin // SBB
				// subtract with borrow
				o_alu_op = 4'h3;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_010_0001: begin // ADD
				// add
				o_alu_op = 4'h0;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_011_0001: begin // ADC
				// add with carry
				o_alu_op = 4'h1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_100_0001: begin // AND
				// and
				o_alu_op = 4'h4;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_101_0001: begin // OR
				// or
				o_alu_op = 4'h5;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_110_0001: begin // NOR
				// nor
				o_alu_op = 4'h6;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_111_0001: begin // XOR
				// xor
				o_alu_op = 4'h7;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_000_0010: begin // SLL
				// shift left logical
				o_alu_op = 4'h8;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_001_0010: begin // SRL
				// shift right logical
				o_alu_op = 4'h9;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_010_0010: begin // SRA
				// shift right arithmetic
				o_alu_op = 4'hA;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_100_0010: begin // SLL immediate
				// shift left logical
				o_alu_op = 4'h8;
				// immediate operand
				o_ig_sel = 3'h0;
				// switch ALU operand B to immediate
				o_alu_b_sel = 1'b1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_101_0010: begin // SRL immediate
				// shift right logical
				o_alu_op = 4'h9;
				// immediate operand
				o_ig_sel = 3'h0;
				// switch ALU operand B to immediate
				o_alu_b_sel = 1'b1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_110_0010: begin // SRA immediate
				// shift right arithmetic
				o_alu_op = 4'hA;
				// immediate operand
				o_ig_sel = 3'h0;
				// switch ALU operand B to immediate
				o_alu_b_sel = 1'b1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_??0_1010: begin // LI
				// ALU passthrough B
				o_alu_op = 4'hC;
				// load immediate
				o_ig_sel = 3'h1;
				// switch ALU operand B to immediate
				o_alu_b_sel = 1'b1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h2;
			end
			16'b???_???_???_???_1011: begin // ADD immediate
				// add
				o_alu_op = 4'h0;
				// immediate operand
				o_ig_sel = 3'h2;
				// switch ALU operand B to immediate
				o_alu_b_sel = 1'b1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h1;
			end
			16'b???_???_???_???_1100: begin // SB
				// operand A
				o_rf_rsel1 = i_wsel;
				// operand B
				o_rf_rsel2 = i_rsel1;
				// output A to mem
				o_mem_wdata_sel = 2'h0;
				o_mem_we = 1'b1;
				// immediate operand
				o_ig_sel = 3'h3;
				// set address + immediate
				o_agu_sel = 3'h1;
			end
			16'b???_???_???_???_1101: begin // LB
				// operand B
				o_rf_rsel2 = i_rsel1;
				// immediate operand
				o_ig_sel = 3'h3;
				// set address + immediate
				o_agu_sel = 3'h1;
				// store result in register
				o_rf_wdata_sel = 2'h2;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 2'h2;
			end
			16'b???_???_???_???_1110: begin // B
				if (w_cond) begin
					// immediate operand
					o_ig_sel = 3'h4;
					// load new PC
					o_agu_sel = 3'h2;
					o_pc_sel = 2'h1;
					o_pc_we = 1'b1;
				end
			end
			16'b???_???_???_???_1111: begin // BL
				o_agu_sel = 3'h2;
				o_mem_wdata_sel = 2'h3;
				o_sp_sel = 2'h2;
				if (w_cond && r_stage == 0) begin
					// store PC upper
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
				end
				if (w_cond && r_stage == 1) begin
					// store PC lower
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					// load new PC
					o_ig_sel = 3'h4;
					o_rf_rsel2 = i_rsel1;
					o_pc_sel = 2'h2;
					o_pc_we = 1'b1;
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
			16'b001_000_000_000_0000: begin // RET
				if (r_stage == 0) r_stage <= 2'h1;
				else if (r_stage == 1) r_stage <= 2'h0;
			end
			16'b???_??1_000_111_0000: begin // BL
				if (r_stage == 0 && w_cond) r_stage <= 2'h1;
				else if (r_stage == 1) r_stage <= 2'h0;
			end
			default: r_stage <= 2'b00;
		endcase

	end

endmodule