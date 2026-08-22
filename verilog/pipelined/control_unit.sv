`timescale 1ns / 1ps

module control_unit (
	input  logic		i_clk,
	input  logic		i_rst,
	input  logic [15:0]	i_instr,
	input  logic [4:0]	i_flags,

	// Interrupt signals
	input  logic		i_int,
	output logic [6:0]	o_int_id,
	output logic		o_int_ack,
	output logic		o_int_id_we,
	output logic		o_int_id_sel,
	input  logic [6:0]	i_id_rdata,

	// Pipeline stall signals
	input  logic		i_stall,

	// Fetch stage IO
	output logic		o_if_stall,

	// Decode stage IO
	output logic [2:0]	o_id_ig_sel,
	input  logic [7:0]	i_id_a,
	output logic		o_cu_flags_sel,

	// Execute stage IO
	output logic [1:0]	o_ex_alu_a_sel,
	output logic		o_ex_alu_b_sel,
	output logic		o_ex_wdata_sel,
	output logic		o_ex_adata_sel,
	output logic [3:0]	o_ex_alu_op,
	output logic		o_ex_adr_sel,

	// Memory stage IO
	output logic		o_mem_re,
	output logic		o_mem_we,

	// WriteBack stage IO
	output logic		o_wb_rf_we,
	output logic		o_wb_cf_we,
	output logic		o_wb_cf_flags_we,
	output logic		o_wb_cf_sp_we,
	output logic		o_wb_pc_lwe,
	output logic		o_wb_pc_uwe,
	output logic		o_wb_pc_we,

	// stagei
	output logic [1:0]	o_stage,
	output logic [2:0]	o_int_stage,
	output logic		o_int,
	output logic		o_hold,
	output logic		o_swi
);

	// stage flip flops and wires
	logic [1:0]	r_stage;

	// interrupt related signals
	logic		w_interrupt;
	logic [2:0]	r_interrupt_stage;
	logic		r_interrupt_internal;
	logic		r_wfi;
	logic		w_illegal_instruction;
	logic		w_cond;

	// fetch stall counter
	logic [1:0]	r_if_stall;
	logic		w_if_stall;
	logic		r_held;
	logic		w_hold;
	logic		w_nohold;
	logic		r_swi;

	assign o_stage = r_stage;
	assign o_int_stage = r_interrupt_stage;
	assign o_int = w_interrupt;
	assign o_swi = r_swi;

	always_comb begin
		// zero initialize default signals
		o_mem_re = 1'b0;
		o_mem_we = 1'b0;
		o_wb_rf_we = 1'b0;
		o_wb_cf_we = 1'b0;
		o_wb_cf_flags_we = 1'b0;
		o_wb_cf_sp_we = 1'b0;
		o_wb_pc_lwe = 1'b0;
		o_wb_pc_uwe = 1'b0;
		o_wb_pc_we = 1'b0;

		o_ex_alu_a_sel = 2'h0;
		o_ex_alu_b_sel = 1'b0;
		o_ex_wdata_sel = 1'b0;
		o_ex_adata_sel = 1'b0;
		o_ex_adr_sel = 1'b0;
		o_ex_alu_op = 4'h0;
		o_id_ig_sel = 3'h0;
		o_cu_flags_sel = 1'b0;

		o_int_id = 7'h0;
		o_int_id_we = 1'b0;
		o_int_id_sel = 1'b0;

		w_interrupt = (i_int && i_flags[4]) || r_interrupt_internal;
		w_illegal_instruction = 1'b0;

		w_if_stall = 1'b0;
		w_hold = 1'b0;

		case (i_instr[15:13])
			3'h0: w_cond = i_flags[1];	// ZS/EQ
			3'h1: w_cond = ~i_flags[1];	// ZC/NE
			3'h2: w_cond = i_flags[0];	// CS/HS
			3'h3: w_cond = ~i_flags[0];	// CC/LO
			3'h4: w_cond = i_flags[2];	// NS/MI
			3'h5: w_cond = ~i_flags[2];	// NC/PL
			3'h6: w_cond = i_flags[3];	// VS
			3'h7: w_cond = 1'b1;		// AL
			default: w_cond = 1'b1;
		endcase

		if (r_stage > 0 || (r_interrupt_stage == 0 && !w_interrupt && !r_wfi)) begin
			casez (i_instr)
				16'b000_000_000_000_0000: ; // NOP
				16'b001_000_000_000_0000: begin // RET
					if (((r_stage == 0) && !r_held) || (r_stage == 1)) begin
						// pre-increment
						o_id_ig_sel = 3'h6; // 1
						o_ex_alu_op = 4'h0; // add
						o_ex_alu_a_sel = 2'h1; // r_idex_adata
						o_ex_alu_b_sel = 1'b1; // imm
						o_ex_adata_sel = 1'b1; // w_ex_alu_result
						o_ex_adr_sel = 1'b1; // w_ex_alu_result
						o_mem_re = 1'b1;
						o_wb_cf_sp_we = 1'b1;
						w_if_stall = 1'b1;
					end
					if ((r_stage == 0) && !r_held) begin
						o_wb_pc_lwe = 1'b1;
						w_hold = 1'b1;
					end else if (r_stage == 1) begin
						o_wb_pc_uwe = 1'b1;
					end
				end
				16'b010_000_000_000_0000: // WFI
					w_if_stall = 1'b1;
				16'b000_001_000_000_0000: begin // RETI
					if (((r_stage == 0) && !r_held) || (r_stage == 1) || (r_stage == 2)) begin
						// pre-increment
						o_id_ig_sel = 3'h6; // 1
						o_ex_alu_op = 4'h0; // add
						o_ex_alu_a_sel = 2'h1; // r_idex_adata
						o_ex_alu_b_sel = 1'b1; // imm
						o_ex_adata_sel = 1'b1; // w_ex_alu_result
						o_ex_adr_sel = 1'b1; // w_ex_alu_result
						o_mem_re = 1'b1;
						o_wb_cf_sp_we = 1'b1;
						w_if_stall = 1'b1;
					end
					if ((r_stage == 0) && !r_held) begin
						o_wb_cf_we = 1'b1;
						o_cu_flags_sel = 1'b1;
						w_hold = 1'b1;
					end else if (r_stage == 1) begin
						o_wb_pc_lwe = 1'b1;
					end else if (r_stage == 2) begin
						o_wb_pc_uwe = 1'b1;
					end
				end
				16'b000_???_001_000_0000: begin // SWI
					o_int_id = i_id_a[6:0];
					o_int_id_sel = 1'b1; // w_cu_int_id
					o_int_id_we = 1'b1;
					w_if_stall = 1'b1;
				end
				16'b001_???_001_000_0000: begin // PUSH
					// post-decrement
					o_id_ig_sel = 3'h6; // 1
					o_ex_alu_op = 4'h2; // subtract
					o_ex_alu_a_sel = 2'h1; // r_idex_adata
					o_ex_alu_b_sel = 1'b1; // imm
					o_ex_adata_sel = 1'b1; // w_ex_alu_result
					o_ex_adr_sel = 1'b0; // r_idex_adata
					o_mem_we = 1'b1;
					o_ex_wdata_sel = 1'b1; // r_idex_a;
					o_wb_cf_sp_we = 1'b1;
				end
				16'b???_000_010_000_0000: begin // POP
					// pre-increment
					o_id_ig_sel = 3'h6; // 1
					o_ex_alu_op = 4'h0; // add
					o_ex_alu_a_sel = 2'h1; // r_idex_adata
					o_ex_alu_b_sel = 1'b1; // imm
					o_ex_adata_sel = 1'b1; // w_ex_alu_result
					o_ex_adr_sel = 1'b1; // w_ex_alu_result
					o_mem_re = 1'b1;
					o_wb_rf_we = 1'b1;
					o_wb_cf_sp_we = 1'b1;
				end
				16'b???_???_000_001_0000: begin // MOV csr->gpr
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'hB; // pass through a
					o_ex_alu_a_sel = 2'h0; // r_idex_a
				end
				16'b???_???_001_001_0000: begin // MOV gpr->csr
					o_wb_cf_we = 1'b1;
					o_ex_alu_op = 4'hB; // pass through a
					o_ex_alu_a_sel = 2'h0; // r_idex_a
				end
				16'b???_???_010_001_0000: begin // INCC
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h1; // add with carry
					o_ex_alu_b_sel = 1'b1; // imm
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_id_ig_sel = 3'h5; // 0
				end
				16'b???_???_011_001_0000: begin // DECB
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h3; // subtract with borrow
					o_ex_alu_b_sel = 1'b1; // imm
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_id_ig_sel = 3'h5; // 0
				end
				16'b000_???_???_010_0000: begin // CMP
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h2; // subtract
					o_ex_alu_b_sel = 1'b0; // r_idex_b
					o_ex_alu_a_sel = 2'h0; // r_idex_a
				end
				16'b001_???_???_010_0000: begin // CMA
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h4; // And
					o_ex_alu_b_sel = 1'b0; // r_idex_b
					o_ex_alu_a_sel = 2'h0; // r_idex_a
				end
				16'b???_000_??0_111_0000: if (w_cond) begin // B reg
					o_wb_pc_we = 1'b1;
					o_ex_adata_sel = 1'b0; // r_idex_adata
					w_if_stall = 1'b1;
				end
				16'b???_000_??1_111_0000: begin // BL reg
					if (((r_stage == 0 && w_cond) && !r_held) || (r_stage == 1)) begin
						// push PC high&low to stack
						o_id_ig_sel = 3'h6; // 1
						o_ex_alu_op = 4'h2; // subtract
						o_ex_alu_a_sel = 2'h1; // r_idex_adata
						o_ex_alu_b_sel = 1'b1; // imm
						o_ex_adata_sel = 1'b1; // w_ex_alu_result
						o_ex_adr_sel = 1'b0; // r_idex_adata
						o_mem_we = 1'b1;
						o_ex_wdata_sel = 1'b1; // r_idex_a;
						o_wb_cf_sp_we = 1'b1;
						w_if_stall = 1'b1;
						w_hold = 1'b1;
					end else if (r_stage > 1) begin
						// branch
						o_ex_adata_sel = 1'b0; // r_idex_adata	
						o_wb_pc_we = 1'b1;
					end
				end
				16'b???_???_???_000_0001: begin // SUB
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h2; // subtract
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_001_0001: begin // SBB
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h3; // subtract with borrow
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_010_0001: begin // ADD
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h0; // add
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_011_0001: begin // ADC
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h1; // add with carry
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_100_0001: begin // AND
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h4; // and
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_101_0001: begin // OR
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h5; // or
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_110_0001: begin // NOR
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h6; // or not
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_111_0001: begin // XOR
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h7; // xor
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_000_0010: begin // SLL
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h8; // logical shift left
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_001_0010: begin // SRL
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h9; // logical shift right
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_010_0010: begin // SRA
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'hA; // arithmetic shift right
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b0; // r_idex_b
				end
				16'b???_???_???_100_0010: begin // SLL imm
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h8; // logical shift left
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h0;
				end
				16'b???_???_???_101_0010: begin // SRL imm
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h9; // logical shift right
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h0;
				end
				16'b???_???_???_110_0010: begin // SRA imm
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'hA; // arithmetic shift right
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h0;
				end
				16'b???_???_???_??0_1010: begin // LI
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'hC; // pass B
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h1;
					o_ex_wdata_sel = 1'b0; // w_ex_alu_result
				end
				16'b???_???_???_???_1011: begin // ADD imm
					o_wb_rf_we = 1'b1;
					o_wb_cf_flags_we = 1'b1;
					o_ex_alu_op = 4'h0; // add
					o_ex_alu_a_sel = 2'h0; // r_idex_a
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h0;
				end
				16'b???_???_???_???_1100: begin // SB
					o_ex_alu_op = 4'h0; // add
					o_ex_alu_a_sel = 2'h1; // r_idex_adata
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h2;
					o_ex_wdata_sel = 1'b1; // r_idex_a
					o_ex_adr_sel = 1'b1; // w_ex_alu_result
					o_mem_we = 1'b1;
				end
				16'b???_???_???_???_1101: begin // LB
					o_wb_rf_we = 1'b1;
					o_ex_alu_op = 4'h0; // add
					o_ex_alu_a_sel = 2'h1; // r_idex_adata
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h3;
					o_ex_wdata_sel = 1'b1; // r_idex_a
					o_ex_adr_sel = 1'b1; // w_ex_alu_result
					o_mem_re = 1'b1;
				end
				16'b???_???_???_???_1110: if (w_cond) begin // B rel
					o_ex_alu_op = 4'h0; // add
					o_ex_alu_a_sel = 2'h2; // r_idex_npc
					o_ex_alu_b_sel = 1'b1; // imm
					o_id_ig_sel = 3'h4;
					o_ex_adata_sel = 1'b1; // w_ex_alu_result
					o_wb_pc_we = 1'b1;
					w_if_stall = 1'b1;
				end
				16'b???_???_???_???_1111: begin // BL rel
					if (((r_stage == 0 && w_cond) && !r_held) || (r_stage == 1)) begin
						// push PC high&low to stack
						o_id_ig_sel = 3'h6; // 1
						o_ex_alu_op = 4'h2; // subtract
						o_ex_alu_a_sel = 2'h1; // r_idex_adata
						o_ex_alu_b_sel = 1'b1; // imm
						o_ex_adata_sel = 1'b1; // w_ex_alu_result
						o_ex_adr_sel = 1'b0; // r_idex_adata
						o_mem_we = 1'b1;
						o_ex_wdata_sel = 1'b1; // r_idex_a;
						o_wb_cf_sp_we = 1'b1;
						w_if_stall = 1'b1;
						w_hold = 1'b1;
					end else if (r_stage > 1) begin
						// branch
						o_ex_alu_op = 4'h0; // add
						o_ex_alu_a_sel = 2'h2; // r_idex_npc
						o_ex_alu_b_sel = 1'b1; // imm
						o_id_ig_sel = 3'h4;
						o_ex_adata_sel = 1'b1; // w_ex_alu_result
						o_wb_pc_we = 1'b1;
						w_if_stall = 1'b1;
					end
				end
				default: begin
					o_int_id = 7'h02; // Illegal instruction
					o_int_id_sel = 1'b1; // w_cu_int_id
					o_int_id_we = 1'b1;
					w_if_stall = 1'b1;
					w_illegal_instruction = 1'b1;
				end
			endcase
		end

		if ((r_stage == 0 && r_interrupt_stage == 0 && w_interrupt) || (r_interrupt_stage == 1) || (r_interrupt_stage == 2)) begin
			// push PC high&low&flags to stack
			o_id_ig_sel = 3'h6; // 1
			o_ex_alu_op = 4'h2; // subtract
			o_ex_alu_a_sel = 2'h1; // r_idex_adata
			o_ex_alu_b_sel = 1'b1; // imm
			o_ex_adata_sel = 1'b1; // w_ex_alu_result
			o_ex_adr_sel = 1'b0; // r_idex_adata
			o_mem_we = 1'b1;
			o_ex_wdata_sel = 1'b1; // r_idex_a;
			o_wb_cf_sp_we = 1'b1;
			w_if_stall = 1'b1;
			w_hold = 1'b1;
			if (!r_interrupt_internal && r_interrupt_stage == 0) begin
				o_int_id_sel = 1'b0;
				o_int_id_we = 1'b1;
			end
		end else if (r_interrupt_stage == 3) begin
			o_ex_adr_sel = 1'b0; // r_idex_adata
			o_mem_re = 1'b1;
			w_if_stall = 1'b1;
			o_wb_pc_lwe = 1'b1;
		end else if (r_interrupt_stage == 4) begin
			o_ex_adr_sel = 1'b0; // r_idex_adata
			o_mem_re = 1'b1;
			w_if_stall = 1'b1;
			o_wb_pc_uwe = 1'b1;
		end

		if (i_stall) begin
			o_mem_re = 1'b0;
			o_mem_we = 1'b0;
			o_wb_rf_we = 1'b0;
			o_wb_cf_we = 1'b0;
			o_wb_cf_flags_we = 1'b0;
			o_wb_cf_sp_we = 1'b0;
			o_wb_pc_lwe = 1'b0;
			o_wb_pc_uwe = 1'b0;
			o_wb_pc_we = 1'b0;
		end

		o_if_stall = (r_if_stall > 2'b0) || w_if_stall || r_wfi || w_interrupt;
		o_hold = (r_stage > 0) || (r_interrupt_stage > 0) || w_hold;
	end

	always_ff @(posedge i_clk) begin
		if (i_rst) begin
			r_stage <= 2'b0;
			r_interrupt_stage <= 3'b0;
			r_interrupt_internal <= 1'b0;
			r_wfi <= 1'b0;
			r_if_stall <= 2'b0;
			r_held <= 1'b0;
			r_swi <= 1'b0;
		end else begin
			if (r_if_stall > 2'h0) begin
				r_if_stall <= r_if_stall - 2'h1;
			end
			if (r_stage > 0 || (r_interrupt_stage == 0 && ~w_interrupt && ~r_wfi)) begin
				casez (i_instr)
					16'b001_000_000_000_0000: begin // RET
						if (r_stage == 0 && !r_held) begin
							r_stage <= 2'h1;
							r_held <= 1'b1;
							r_if_stall <= 2'h3;
						end else if (r_stage > 0) begin
							r_stage <= 2'h0;
							r_if_stall <= 2'h3;
						end
					end
					16'b010_000_000_000_0000: // WFI
						r_wfi <= 1'b1;
					16'b000_001_000_000_0000: begin // RETI
						if (r_stage == 0 && !r_held) begin
							r_stage <= 2'h1;
							r_held <= 1'b1;
							r_if_stall <= 2'h3;
						end else if (r_stage == 1) begin
							r_stage <= 2'h2;
						end else if (r_stage == 2) begin
							r_stage <= 2'h3;
						end else if (r_stage > 2) begin
							r_stage <= 2'h0;
							r_if_stall <= 2'h3;
						end
					end
					16'b000_???_001_000_0000: begin // SWI
						r_interrupt_internal <= 1'b1;
						r_stage <= 2'h0;
						r_if_stall <= 2'h3;
						r_swi <= 1'b1;
					end
					16'b???_000_??0_111_0000: // B reg
						if (w_cond)
							r_if_stall <= 2'h3;
					16'b???_000_??1_111_0000: begin // BL reg
						if ((r_stage == 0) && w_cond && !r_held) begin
							r_stage <= 2'h1;
							r_held <= 1'b1;
							r_if_stall <= 2'h3;
						end else if (r_stage == 1) begin
							r_stage <= 2'h2;
						end else if (r_stage > 1) begin
							r_stage <= 2'h0;
							r_if_stall <= 2'h3;
						end
					end
					16'b???_???_???_???_1110: // B rel
						if (w_cond)
							r_if_stall <= 2'h3;
					16'b???_???_???_???_1111: begin // BL rel
						if ((r_stage == 0) && w_cond && !r_held) begin
							r_stage <= 2'h1;
							r_held <= 1'b1;
							r_if_stall <= 2'h3;
						end else if (r_stage == 1) begin
							r_stage <= 2'h2;
						end else if (r_stage > 1) begin
							r_stage <= 2'h0;
							r_if_stall <= 2'h3;
						end
					end
					default: r_stage <= 2'h0;
				endcase
			end

			if (w_illegal_instruction) begin
				r_interrupt_internal <= 1'b1;
				r_if_stall <= 2'h3;
			end

			if (r_stage == 0 && r_interrupt_stage == 0 && w_interrupt) begin
				r_interrupt_stage <= 3'h1;
				if (!r_interrupt_internal) begin
					o_int_ack <= 1'b1;
				end
				r_wfi <= 1'b0;
				r_interrupt_internal <= 1'b0;
				r_if_stall <= 2'h3;
			end else if (r_interrupt_stage == 1) begin
				r_interrupt_stage <= 3'h2;
				o_int_ack <= 1'b0;
			end else if (r_interrupt_stage == 2) begin
				r_interrupt_stage <= 3'h3;
				r_swi <= 1'b0;
			end else if (r_interrupt_stage == 3) begin
				r_interrupt_stage <= 3'h4;
			end else if (r_interrupt_stage == 4) begin
				r_interrupt_stage <= 3'h0;
				r_if_stall <= 2'h3;
			end

			if (r_stage == 0 && r_if_stall == 2'h0) begin
				if (r_held) begin
					r_held <= 1'b0;
				end
			end
		end
	end

endmodule
