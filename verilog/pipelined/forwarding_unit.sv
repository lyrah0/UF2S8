`timescale 1ns / 1ps

module forwarding_unit (
	input  logic [15:0]	i_instr,

	// Pipeline feedback inputs
	input  logic		i_ex_rf_we,
	input  logic [2:0]	i_ex_rf_wsel,
	input  logic		i_mem_rf_we,
	input  logic [2:0]	i_mem_rf_wsel,
	input  logic		i_wb_rf_we,
	input  logic [2:0]	i_wb_rf_wsel,

	input  logic		i_ex_cf_we,
	input  logic [2:0]	i_ex_cf_wsel,
	input  logic		i_mem_cf_we,
	input  logic [2:0]	i_mem_cf_wsel,
	input  logic		i_wb_cf_we,
	input  logic [2:0]	i_wb_cf_wsel,

	input  logic		i_ex_cf_sp_we,
	input  logic		i_mem_cf_sp_we,
	input  logic		i_wb_cf_sp_we,

	input  logic		i_ex_cf_flags_we,
	input  logic		i_mem_cf_flags_we,
	input  logic		i_wb_cf_flags_we,

	input  logic		i_ex_mem_re,

	// Forwarding selectors
	output logic [4:0]	o_id_a_sel,
	output logic [2:0]	o_id_b_sel,
	output logic [2:0]	o_id_adata_sel,
	output logic [3:0]	o_id_adata_selp,
	output logic [2:0]	o_id_flags_sel,

	// Load-use stall output
	output logic		o_stall,

	// CU stage
	input logic [1:0]	i_stage,
	input logic [2:0]	i_int_stage,
	input logic		i_int,
	input logic		i_swi
);

	logic w_rsel1_read;
	logic w_rsel2_read;
	logic [2:0] rsel1;
	logic [2:0] rsel2;
	assign rsel1 = i_instr[12:10];
	assign rsel2 = i_instr[9:7];
	logic [2:0] arsel1;
	logic [2:0] arsel2;
	assign arsel1 = {i_instr[9:8], 1'b0};
	assign arsel2 = {i_instr[9:8], 1'b1};
	logic w_is_sp_op;
	logic w_flags_read;
	assign w_flags_read = (i_instr[3:0] == 4'hE && i_instr[15:13] != 3'h7) ||
			      (i_instr[3:0] == 4'hF && i_instr[15:13] != 3'h7) ||
			      (i_instr[7:0] == 8'b1_111_0000 && i_instr[15:13] != 3'h7) ||
			      (i_instr[9:0] == 10'b000_001_0000 && i_instr[12:10] == 3'h0);

	always_comb begin
		o_stall = 1'b0;
		o_id_adata_sel = 3'h0;
		o_id_adata_selp = 4'h0;

		casez (i_instr)
			16'b000_000_000_000_0000: w_rsel1_read = 1'b0; // NOP
			16'b001_000_000_000_0000: w_rsel1_read = 1'b0; // RET
			16'b010_000_000_000_0000: w_rsel1_read = 1'b0; // WFI
			16'b000_001_000_000_0000: w_rsel1_read = 1'b0; // RETI
			16'b???_000_??0_111_0000: w_rsel1_read = 1'b0; // B reg
			16'b???_000_??1_111_0000: w_rsel1_read = 1'b0; // BL reg
			16'b???_???_???_??0_1010: w_rsel1_read = 1'b0; // LI
			16'b???_???_???_???_1110: w_rsel1_read = 1'b0; // B rel
			16'b???_???_???_???_1111: w_rsel1_read = 1'b0; // BL rel
			default:                  w_rsel1_read = 1'b1;
		endcase

		casez (i_instr)
			16'b???_???_???_000_0001: w_rsel2_read = 1'b1; // SUB
			16'b???_???_???_001_0001: w_rsel2_read = 1'b1; // SBB
			16'b???_???_???_010_0001: w_rsel2_read = 1'b1; // ADD
			16'b???_???_???_011_0001: w_rsel2_read = 1'b1; // ADC
			16'b???_???_???_100_0001: w_rsel2_read = 1'b1; // AND
			16'b???_???_???_101_0001: w_rsel2_read = 1'b1; // OR
			16'b???_???_???_110_0001: w_rsel2_read = 1'b1; // NOR
			16'b???_???_???_111_0001: w_rsel2_read = 1'b1; // XOR
			16'b???_???_???_000_0010: w_rsel2_read = 1'b1; // SLL
			16'b???_???_???_001_0010: w_rsel2_read = 1'b1; // SRL
			16'b???_???_???_010_0010: w_rsel2_read = 1'b1; // SRA
			16'b000_???_???_010_0000: w_rsel2_read = 1'b1; // CMP
			16'b001_???_???_010_0000: w_rsel2_read = 1'b1; // CMA
			16'b???_???_???_???_1100: w_rsel2_read = 1'b1; // SB
			16'b???_???_???_???_1101: w_rsel2_read = 1'b1; // LB
			default:                  w_rsel2_read = 1'b0;
		endcase

		// Override for CSR read
		if (i_instr[9:0] == 10'b000_001_0000) begin // MOV csr->gpr
			// special handling for SP first
			if (i_ex_cf_sp_we && (i_ex_cf_wsel == 3'h6)) begin
				o_id_a_sel = 5'h5; // Forward from EX adata low
			end else if (i_mem_cf_sp_we && (i_mem_cf_wsel == 3'h6)) begin
				o_id_a_sel = 5'h6; // Forward from MEM adata low
			end else if (i_wb_cf_sp_we && (i_wb_cf_wsel == 3'h6)) begin
				o_id_a_sel = 5'h7; // Forward from WB adata low
			end else if (i_ex_cf_sp_we && (i_ex_cf_wsel == 3'h7)) begin
				o_id_a_sel = 5'h8; // Forward from EX adata high
			end else if (i_mem_cf_sp_we && (i_mem_cf_wsel == 3'h7)) begin
				o_id_a_sel = 5'h9; // Forward from MEM adata high
			end else if (i_wb_cf_sp_we && (i_wb_cf_wsel == 3'h7)) begin
				o_id_a_sel = 5'hA; // Forward from WB adata high
			// Special handling for flags
			end else if (3'h0 == rsel1) begin
				o_id_a_sel = 5'h10;
			// Normal register operations
			end else if (i_ex_cf_we && (i_ex_cf_wsel == rsel1)) begin
				o_id_a_sel = 5'h2; // Forward from EX
				if (i_ex_mem_re) o_stall = 1'b1;
			end else if (i_mem_cf_we && (i_mem_cf_wsel == rsel1)) begin
				o_id_a_sel = 5'h3; // Forward from MEM
			end else if (i_wb_cf_we && (i_wb_cf_wsel == rsel1)) begin
				o_id_a_sel = 5'h4; // Forward from WB
			end else begin
				o_id_a_sel = 5'h1;
			end
		// Operand A selector
		end else if (w_rsel1_read) begin
			if (i_ex_rf_we && (i_ex_rf_wsel == rsel1)) begin
				o_id_a_sel = 5'h2; // Forward from EX
				if (i_ex_mem_re) o_stall = 1'b1;
			end else if (i_mem_rf_we && (i_mem_rf_wsel == rsel1)) begin
				o_id_a_sel = 5'h3; // Forward from MEM
			end else if (i_wb_rf_we && (i_wb_rf_wsel == rsel1)) begin
				o_id_a_sel = 5'h4; // Forward from WB
			end else begin
				o_id_a_sel = 5'h0; // from register file
			end
		// special A handling for branch and link instructions
		end else if (((i_instr[7:0] == 8'b1_111_0000) && (i_instr[12:10] == 3'b000)) ||
		(i_instr[3:0] == 4'hF)) begin
			case(i_stage)
				2'h0: o_id_a_sel = 5'h14; // i_npc[15:8]
				2'h1: o_id_a_sel = 5'h13; // i_npc[7:1]
				default: o_id_a_sel = 5'h0;
			endcase
		end else if (i_int || (i_int_stage > 0)) begin
			case(i_int_stage)
				3'h0: o_id_a_sel = 5'h14; // i_npc[15:8]
				3'h1: o_id_a_sel = 5'h13; // i_npc[7:1]
				3'h2: o_id_a_sel = 5'h10; // flags
				default: o_id_a_sel = 5'h0;
			endcase
		end else begin
			o_id_a_sel = 5'h0;
		end

		
		// Operand B selector
		if (w_rsel2_read) begin
			if (i_ex_rf_we && (i_ex_rf_wsel == rsel2)) begin
				o_id_b_sel = 3'h1; // Forward from EX
				if (i_ex_mem_re) o_stall = 1'b1;
			end else if (i_mem_rf_we && (i_mem_rf_wsel == rsel2)) begin
				o_id_b_sel = 3'h2; // Forward from MEM
			end else if (i_wb_rf_we && (i_wb_rf_wsel == rsel2)) begin
				o_id_b_sel = 3'h3; // Forward from WB
			end else begin
				o_id_b_sel = 3'h0; // from register file
			end
		end else begin
			o_id_b_sel = 3'h0;
		end

		// Address pair forwarding
		casez (i_instr)
			16'b001_???_001_000_0000: w_is_sp_op = 1'b1; // PUSH
			16'b???_000_010_000_0000: w_is_sp_op = 1'b1; // POP
			16'b001_000_000_000_0000: w_is_sp_op = i_stage < 2'h2; // RET
			16'b000_001_000_000_0000: w_is_sp_op = i_stage < 2'h3; // RETI
			16'b???_000_??1_111_0000: w_is_sp_op = i_stage < 2'h2; // BL reg
			16'b???_???_???_???_1111: w_is_sp_op = i_stage < 2'h2; // BL rel
			default:                  w_is_sp_op = 1'b0;
		endcase

		if (i_int_stage > 2) begin
			case(i_int_stage)
				3'h3: o_id_adata_sel = 3'h5; // r_int_id lowi
				3'h4: o_id_adata_sel = 3'h6; // r_int_id high
				default: o_id_adata_sel = 3'h0;
			endcase
			o_id_adata_selp = 4'h0; // w_id_adata
		end else if (w_is_sp_op || i_int || (i_int_stage > 0)) begin
			if (i_ex_cf_sp_we)
				o_id_adata_sel = 3'h2; // i_ex_adata
			else if (i_mem_cf_sp_we)
				o_id_adata_sel = 3'h3; // i_mem_adata
			else if (i_wb_cf_sp_we)
				o_id_adata_sel = 3'h4; // i_wb_adata
			else
				o_id_adata_sel = 3'h1; // Stack Pointer
			
			if (i_ex_cf_we && (i_ex_cf_wsel == 3'h6)) begin
				if (i_mem_cf_we && (i_mem_cf_wsel == 3'h7)) begin
					o_id_adata_selp = 4'h9; // low byte from EX, high byte from MEM
				end else if (i_wb_cf_we && (i_wb_cf_wsel == 3'h7)) begin
					o_id_adata_selp = 4'hB; // low byte from EX, high byte from WB
				end else begin
					o_id_adata_selp = 4'h1; // low byte from EX
				end
				if (i_ex_mem_re) o_stall = 1'b1;
			end else if (i_mem_cf_we && (i_mem_cf_wsel == 3'h6)) begin
				if (i_ex_cf_we && (i_ex_cf_wsel == 3'h7)) begin
					o_id_adata_selp = 4'h7; // low byte from MEM, high byte from EX
					if (i_ex_mem_re)
						o_stall = 1'b1;
				end else if (i_wb_cf_we && (i_wb_cf_wsel == 3'h7)) begin
					o_id_adata_selp = 4'hC; // low byte from MEM, high byte from WB
				end else begin
					o_id_adata_selp = 4'h2; // low byte from MEM
				end
			end else if (i_wb_cf_we && (i_wb_cf_wsel == 3'h6)) begin
				if (i_ex_cf_we && (i_ex_cf_wsel == 3'h7)) begin
					o_id_adata_selp = 4'h8; // low byte from WB, high byte from EX
					if (i_ex_mem_re)
						o_stall = 1'b1;
				end else if (i_mem_cf_we && (i_mem_cf_wsel == 3'h7)) begin
					o_id_adata_selp = 4'hA; // low byte from WB, high byte from MEM
				end else begin
					o_id_adata_selp = 4'h3; // low byte from WB
				end
			end else if (i_ex_cf_we && (i_ex_cf_wsel == 3'h7)) begin
				o_id_adata_selp = 4'h4; // high byte from EX
				if (i_ex_mem_re)
					o_stall = 1'b1;
			end else if (i_mem_cf_we && (i_mem_cf_wsel == 3'h7)) begin
				o_id_adata_selp = 4'h5; // high byte from MEM
			end else if (i_wb_cf_we && (i_wb_cf_wsel == 3'h7)) begin
				o_id_adata_selp = 4'h6; // high byte from WB
			end
		end else begin
			if (i_ex_rf_we && (i_ex_rf_wsel == arsel1)) begin
				if (i_mem_rf_we && (i_mem_rf_wsel == arsel2))
					o_id_adata_selp = 4'h9; // low byte from EX, high byte from MEM
				else if (i_wb_rf_we && (i_wb_rf_wsel == arsel2))
					o_id_adata_selp = 4'hB; // low byte from EX, high byte from WB
				else
					o_id_adata_selp = 4'h1; // low byte from EX
				if (i_ex_mem_re)
					o_stall = 1'b1;
			end else if (i_mem_rf_we && (i_mem_rf_wsel == arsel1)) begin
				if (i_ex_rf_we && (i_ex_rf_wsel == arsel2)) begin
					o_id_adata_selp = 4'h7; // low byte from MEM, high byte from EX
					if (i_ex_mem_re)
						o_stall = 1'b1;
				end else if (i_wb_rf_we && (i_wb_rf_wsel == arsel2))
					o_id_adata_selp = 4'hC; // low byte from MEM, high byte from WB
				else
					o_id_adata_selp = 4'h2; // low byte from MEM
			end else if (i_wb_rf_we && (i_wb_rf_wsel == arsel1)) begin
				if (i_ex_rf_we && (i_ex_rf_wsel == arsel2)) begin
					o_id_adata_selp = 4'h8; // low byte from WB, high byte from EX
					if (i_ex_mem_re)
						o_stall = 1'b1;
				end else if (i_mem_rf_we && (i_mem_rf_wsel == arsel2))
					o_id_adata_selp = 4'hA; // low byte from WB, high byte from MEM
				else
					o_id_adata_selp = 4'h3; // low byte from WB
			end else if (i_ex_rf_we && (i_ex_rf_wsel == arsel2)) begin
				o_id_adata_selp = 4'h4; // high byte from EX
				if (i_ex_mem_re)
					o_stall = 1'b1;
			end else if (i_mem_rf_we && (i_mem_rf_wsel == arsel2)) begin
				o_id_adata_selp = 4'h5; // high byte from MEM
			end else if (i_wb_rf_we && (i_wb_rf_wsel == arsel2)) begin
				o_id_adata_selp = 4'h6; // high byte from WB
			end
		end

		// flags forwarding
		if (i_ex_cf_wsel == 0 && i_ex_cf_we) begin
			o_id_flags_sel = 3'h4;
			if (i_ex_mem_re && w_flags_read) o_stall = 1'b1;
		end else if (i_mem_cf_we && (i_mem_cf_wsel == 0)) begin
			o_id_flags_sel = 3'h5;
		end else if (i_wb_cf_we && (i_wb_cf_wsel == 0)) begin
			o_id_flags_sel = 3'h6;
		end else if (i_ex_cf_flags_we) begin
			o_id_flags_sel = 3'h1;
		end else if (i_mem_cf_flags_we) begin
			o_id_flags_sel = 3'h2;
		end else if (i_wb_cf_flags_we) begin
			o_id_flags_sel = 3'h3;
		end else begin
			o_id_flags_sel = 3'h0;
		end
	end

endmodule
