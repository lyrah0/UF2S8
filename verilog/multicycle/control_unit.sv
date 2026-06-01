`timescale 1ns / 1ps
module control_unit(
        // Clock and reset
	input  logic        i_clk,
	input  logic        i_rst,

        // instruction
	input  logic [15:0] i_instruction,
        
        // Flags,
        input  logic [4:0]  i_flags,

        // Wishbone
        output logic        o_wb_we,
	output logic        o_wb_req,
	output logic        o_wb_lock,
        input  logic        i_wb_ready,
        
        // Register File
	output logic        o_rf_we,
	
	// CSR File
	output logic        o_cf_we,
	output logic [2:0]  o_cf_rsel,
	output logic [2:0]  o_cf_wsel,
	output logic        o_cf_wsp,

        // ALU
        output logic [1:0]  o_alu_a_sel,
        output logic        o_alu_b_sel,
        output logic [3:0]  o_alu_op,
	
	// immediate generator
	output logic [2:0]  o_ig_sel,

	// top level signals
	output logic        o_ctr_pc_we,
	output logic [1:0]  o_ctr_pc_sel,
	output logic [2:0]  o_ctr_address_sel,
	output logic        o_ctr_address_we,
	output logic        o_ctr_data_we,
	output logic        o_ctr_instruction_lwe,
	output logic        o_ctr_instruction_uwe,
	output logic [2:0]  o_ctr_cf_wdata_sel,
	output logic [1:0]  o_ctr_rf_wdata_sel,
	output logic [1:0]  o_ctr_data_sel,

	// Interrupt Interface
	input  logic		i_interrupt,
	output logic [6:0]	o_interrupt_id,
	output logic		or_interrupt_ack,
	output logic		o_interrupt_id_we,
	output logic		o_interrupt_id_sel,
	input logic [6:0]	i_rdata

);

	// stage flip flops and wires
	logic [2:0]	r_stage;

	// interrupt related signals
	logic		w_interrupt;
	logic [2:0]	r_interrupt_stage;
	logic		r_interrupt_internal;
	logic		r_wfi;
	logic		w_illegal_instruction;

	logic		w_cond;

	always_comb begin
		w_interrupt = (i_interrupt && i_flags[4]) || r_interrupt_internal;
		w_illegal_instruction = 1'b0;

		o_wb_we = 1'b0;
		o_wb_req = 1'b0;
		o_wb_lock = 1'b0;
		o_rf_we = 1'b0;
		o_cf_we = 1'b0;
		o_cf_rsel = i_instruction[12:10];
		o_cf_wsel = 3'h0;
		o_cf_wsp = 1'b0;
		o_alu_a_sel = 2'h0;
		o_alu_b_sel = 1'b0;
		o_alu_op = 4'h0;
		o_ig_sel = 3'h0;
		o_ctr_pc_we = 1'b0;
		o_ctr_pc_sel = 2'h0;
		o_ctr_address_sel = 3'h0;
		o_ctr_address_we = 1'b0;
		o_ctr_data_we = 1'b0;
		o_ctr_instruction_lwe = 1'h0;
		o_ctr_instruction_uwe = 1'h0;
		o_ctr_cf_wdata_sel = 3'h0;
		o_ctr_rf_wdata_sel = 2'h0;
		o_ctr_data_sel = 2'h0;
		o_interrupt_id = 7'h0;
		o_interrupt_id_we = 1'b0;
		o_interrupt_id_sel = 1'b0;

		case (i_instruction[15:13])
			3'h0: w_cond = i_flags[1];	// ZS/EQ
			3'h1: w_cond = ~i_flags[1];	// ZC/NE
			3'h2: w_cond = i_flags[0];	// CS/HS
			3'h3: w_cond = ~i_flags[0];	// CC/LO
			3'h4: w_cond = i_flags[2];	// NS/MI
			3'h5: w_cond = ~i_flags[2];	// NC/PL
			3'h6: w_cond = i_flags[3];	// VS
			3'h7: w_cond = 1'b1;		// AL
		endcase

		if (r_stage == 0 && r_interrupt_stage == 0 && ~w_interrupt && ~r_wfi) begin // Fetch instruction byte 1
			// request lower byte of instruction
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_ctr_address_sel = 3'h2;
			o_ctr_address_we = 1'b1;
		end
		if (r_stage == 1 && i_wb_ready) begin
			// request upper byte of instruction and write lower
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_ctr_address_sel = 3'h3;
			o_ctr_address_we = 1'b1;
			o_ctr_instruction_lwe = 1'b1;
		end
		if (r_stage == 2 && i_wb_ready) begin
			// disable request and write upper
			o_wb_req = 1'b0;
			o_wb_lock = 1'b0;
			o_ctr_instruction_uwe = 1'b1;
			// update PC
			o_alu_a_sel = 2'h2;
			o_alu_b_sel = 1'b1;
			o_ig_sel = 3'h7;
			o_ctr_pc_we = 1'b1;
			o_ctr_pc_sel = 2'h0;
		end

		if (r_stage > 2) begin
			casez (i_instruction)
				16'b000_000_000_000_0000: ; // NOP
				16'b001_000_000_000_0000: begin // RET
					if (r_stage == 3) begin
						// increment stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h0;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						// increment stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h0;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// keep request up
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
						// write PC lower
						o_ctr_pc_we = 1'b1;
						o_ctr_pc_sel = 2'h2;
					end
					if (r_stage == 5 && i_wb_ready) begin
						// write PC upper
						o_ctr_pc_we = 1'b1;
						o_ctr_pc_sel = 2'h3;
					end
				end
				16'b010_000_000_000_0000: ; // WFI
				16'b000_001_000_000_0000: begin // RETI
					if (r_stage == 3) begin
						// increment stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h0;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						// increment stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h0;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// keep request up
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
						// write flags
						o_ctr_cf_wdata_sel = 3'h2;
						o_cf_wsel = 3'h0;
						o_cf_we = 1'b1;
					end
					if (r_stage == 5 && i_wb_ready) begin
						// increment stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h0;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// keep request up
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
						// write PC lower
						o_ctr_pc_we = 1'b1;
						o_ctr_pc_sel = 2'h2;
					end
					if (r_stage == 6 && i_wb_ready) begin
						// write PC upper
						o_ctr_pc_we = 1'b1;
						o_ctr_pc_sel = 2'h3;
					end
				end
				16'b000_???_001_000_0000: begin // SWI
					o_interrupt_id = i_rdata;
					o_interrupt_id_sel = 1'b1;
					o_interrupt_id_we = 1'b1;
				end
				16'b001_???_001_000_0000: begin // PUSH
					if (r_stage == 3) begin
						// decrement stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h2;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b0;
						o_wb_we = 1'b1;
						o_ctr_address_sel = 3'h1;
						o_ctr_address_we = 1'b1;
						// send data
						o_ctr_data_sel = 2'h0;
						o_ctr_data_we = 1'b1;
					end
				end
				16'b???_000_010_000_0000: begin // POP
					if (r_stage == 3) begin
						// increment stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h0;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b0;
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						// disable request
						o_wb_req = 1'b0;
						o_wb_lock = 1'b0;
						// write register
						o_ctr_rf_wdata_sel = 2'h1;
						o_rf_we = 1'b1;
					end
				end
				16'b???_???_000_001_0000: begin // MOV csr->gpr
					// pick csr
					o_cf_rsel = i_instruction[12:10];
					// write register
					o_ctr_rf_wdata_sel = 2'h2;
					o_rf_we = 1'b1;
				end
				16'b???_???_001_001_0000: begin // MOV gpr->csr
					// write csr
					o_ctr_cf_wdata_sel = 3'h1;
					o_cf_wsel = i_instruction[15:13];
					o_cf_we = 1'b1;
				end
				16'b???_???_010_001_0000: begin // INCC
					// select operation and inputs
					o_alu_op = 4'h1;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'b1;
					o_ig_sel = 3'h5;
					// write register
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_011_001_0000: begin // DECB
					// select operation and inputs
					o_alu_op = 4'h3;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'b1;
					o_ig_sel = 3'h5;
					// write register
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b000_???_???_010_0000: begin // CMP
					// select operation and inputs
					o_alu_op = 4'h2;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b001_???_???_010_0000: begin // CMA
					// select operation and inputs
					o_alu_op = 4'h4;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_000_??0_111_0000: if (w_cond) begin // B
					// write PC
					o_ctr_pc_sel = 2'h1;
					o_ctr_pc_we = 1'b1;
				end
				16'b???_000_??1_111_0000: begin // BL
					if (r_stage == 3 && w_cond) begin
						// decrement stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h2;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_wb_we = 1'b1;
						o_ctr_address_sel = 3'h1;
						o_ctr_address_we = 1'b1;
						// send PC upper
						o_ctr_data_sel = 2'h3;
						o_ctr_data_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						// decrement stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h2;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_wb_we = 1'b1;
						o_ctr_address_sel = 3'h1;
						o_ctr_address_we = 1'b1;
						// send PC lower
						o_ctr_data_sel = 2'h2;
						o_ctr_data_we = 1'b1;
						// write new address to PC
						o_ctr_pc_sel = 2'h1;
						o_ctr_pc_we = 1'b1;
					end
				end
				16'b???_???_???_000_0001: begin // SUB
					// select alu op and inputs
					o_alu_op = 4'h2;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_001_0001: begin // SBB
					// select alu op and inputs
					o_alu_op = 4'h3;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_010_0001: begin // ADD
					// select alu op and inputs
					o_alu_op = 4'h0;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_011_0001: begin // ADC
					// select alu op and inputs
					o_alu_op = 4'h1;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_100_0001: begin // AND
					// select alu op and inputs
					o_alu_op = 4'h4;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_101_0001: begin // OR
					// select alu op and inputs
					o_alu_op = 4'h5;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_110_0001: begin // NOR
					// select alu op and inputs
					o_alu_op = 4'h6;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_111_0001: begin // XOR
					// select alu op and inputs
					o_alu_op = 4'h7;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_000_0010: begin // SLL
					// select alu op and inputs
					o_alu_op = 4'h8;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_001_0010: begin // SRL
					// select alu op and inputs
					o_alu_op = 4'h9;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_010_0010: begin // SRA
					// select alu op and inputs
					o_alu_op = 4'hA;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_100_0010: begin // SLL
					// select alu op and inputs
					o_alu_op = 4'h8;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h1;
					o_ig_sel = 3'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_101_0010: begin // SRL
					// select alu op and inputs
					o_alu_op = 4'h9;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h1;
					o_ig_sel = 3'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_110_0010: begin // SRA
					// select alu op and inputs
					o_alu_op = 4'hA;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h1;
					o_ig_sel = 3'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_??0_1010: begin // LI
					// select and output immediate
					o_alu_op = 4'hC;
					o_alu_b_sel = 1'h1;
					o_ig_sel = 3'h1;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_???_1011: begin // ADD imm
					// select alu op and inputs
					o_alu_op = 4'h0;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'h1;
					o_ig_sel = 3'h0;
					// select register input and write enable
					o_ctr_rf_wdata_sel = 2'h0;
					o_rf_we = 1'b1;
					// write flags
					o_ctr_cf_wdata_sel = 3'h0;
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_???_???_???_1100: begin // SB
					if (r_stage == 3) begin
						// request to write to bus
						o_wb_req = 1'b1;
						o_wb_we = 1'b1;
						// address
						o_alu_op = 4'h0;
						o_alu_a_sel = 2'h1;
						o_alu_b_sel = 1'h1;
						o_ig_sel = 3'h2;
						// select address and write enable
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
						// select data and write enable
						o_ctr_data_sel = 2'h0;
						o_ctr_data_we = 1'b1;
					end
				end
				16'b???_???_???_???_1101: begin // LB
					if (r_stage == 3) begin
						// request to write to bus
						o_wb_req = 1'b1;
						// address
						o_alu_op = 4'h0;
						o_alu_a_sel = 2'h1;
						o_alu_b_sel = 1'h1;
						o_ig_sel = 3'h3;
						// select address and write enable
						o_ctr_address_sel = 3'h0;
						o_ctr_address_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						o_ctr_rf_wdata_sel = 2'h1;
						o_rf_we = 1'b1;
					end
				end
				16'b???_???_???_???_1110: begin // B
					if (w_cond) begin
						// select PC and add offset
						o_alu_op = 4'h0;
						o_alu_a_sel = 2'h2;
						o_alu_b_sel = 1'h1;
						o_ig_sel = 3'h4;
						// write PC
						o_ctr_pc_sel = 2'h0;
						o_ctr_pc_we = 1'b1;
					end
				end
				16'b???_???_???_???_1111: begin // BL
					if (r_stage == 3 && w_cond) begin
						// decrement stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h2;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_wb_we = 1'b1;
						o_ctr_address_sel = 3'h1;
						o_ctr_address_we = 1'b1;
						// send PC upper
						o_ctr_data_sel = 2'h3;
						o_ctr_data_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						// decrement stack pointer
						o_alu_a_sel = 2'h3;
						o_alu_b_sel = 1'b1;
						o_alu_op = 4'h2;
						o_ig_sel = 3'h6;
						o_cf_wsp = 1'b1;
						// send request to bus
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_wb_we = 1'b1;
						o_ctr_address_sel = 3'h1;
						o_ctr_address_we = 1'b1;
						// send PC lower
						o_ctr_data_sel = 2'h2;
						o_ctr_data_we = 1'b1;
					end
					if (r_stage == 5 && i_wb_ready) begin
						// select PC and add offset
						o_alu_op = 4'h0;
						o_alu_a_sel = 2'h2;
						o_alu_b_sel = 1'h1;
						o_ig_sel = 3'h4;
						// write PC
						o_ctr_pc_sel = 2'h0;
						o_ctr_pc_we = 1'b1;
					end
				end
				default: w_illegal_instruction = 1'b1;
			endcase
		end

		if (r_stage == 0 && r_interrupt_stage == 0 && w_interrupt) begin
			// decrement stack pointer
			o_alu_a_sel = 2'h3;
			o_alu_b_sel = 1'b1;
			o_alu_op = 4'h2;
			o_ig_sel = 3'h6;
			o_cf_wsp = 1'b1;
			// request
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_wb_we = 1'b1;
			// SP pre to r_address
			o_ctr_address_sel = 3'h1;
			o_ctr_address_we = 1'b1;
			// PC upper to r_data
			o_ctr_data_sel = 2'h3;
			o_ctr_data_we = 1'b1;
			if (~r_interrupt_internal) begin
				// write interrupt ID to r_int_id
				o_interrupt_id_sel = 1'b0;
				o_interrupt_id_we = 1'b1;
			end
		end else if (r_interrupt_stage == 1 && i_wb_ready) begin
			// decrement stack pointer
			o_alu_a_sel = 2'h3;
			o_alu_b_sel = 1'b1;
			o_alu_op = 4'h2;
			o_ig_sel = 3'h6;
			o_cf_wsp = 1'b1;
			// request
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_wb_we = 1'b1;
			// SP pre to r_address
			o_ctr_address_sel = 3'h1;
			o_ctr_address_we = 1'b1;
			// PC lower to r_data
			o_ctr_data_sel = 2'h2;
			o_ctr_data_we = 1'b1;
		end else if (r_interrupt_stage == 2 && i_wb_ready) begin
			// decrement stack pointer
			o_alu_a_sel = 2'h3;
			o_alu_b_sel = 1'b1;
			o_alu_op = 4'h2;
			o_ig_sel = 3'h6;
			o_cf_wsp = 1'b1;
			// request
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_wb_we = 1'b1;
			// SP pre to r_address
			o_ctr_address_sel = 3'h1;
			o_ctr_address_we = 1'b1;
			// flags to r_data
			o_ctr_data_sel = 2'h1;
			o_ctr_data_we = 1'b1;
			// disable interrupts
			o_cf_wsel = 3'h0;
			o_cf_we = 1'b1;
			o_ctr_cf_wdata_sel = 3'h3;
		end else if (r_interrupt_stage == 3 && i_wb_ready) begin
			// request
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			// interrupt vector lower to address
			o_ctr_address_sel = 3'h4;
			o_ctr_address_we = 1'b1;
		end else if (r_interrupt_stage == 4 && i_wb_ready) begin
			// request
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			// interrupt vector upper to address
			o_ctr_address_sel = 3'h5;
			o_ctr_address_we = 1'b1;
			// write PC lower
			o_ctr_pc_we = 1'b1;
			o_ctr_pc_sel = 2'h2;
		end else if (r_interrupt_stage == 5 && i_wb_ready) begin
			// write PC upper
			o_ctr_pc_we = 1'b1;
			o_ctr_pc_sel = 2'h3;
		end
	end

	always_ff @(posedge i_clk) begin
		if (i_rst) begin
			r_stage <= 3'b0;
			r_interrupt_stage <= 3'b0;
			r_interrupt_internal <= 1'b0;
			r_wfi <= 1'b0;
		end else begin
			if (r_stage == 0 && r_interrupt_stage == 0 && ~w_interrupt && ~r_wfi) begin
				r_stage <= 3'h1;
			end
			if (r_stage == 1 && i_wb_ready) begin
				r_stage <= 3'h2;
			end
			if (r_stage == 2 && i_wb_ready) begin
				r_stage <= 3'h3;
			end
			if (r_stage > 2) begin
				casez (i_instruction)
					16'b000_000_000_000_0000: r_stage <= 3'h0; // NOP
					16'b001_000_000_000_0000: begin // RET
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h5;
						end
						if (r_stage == 5 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b010_000_000_000_0000: begin // WFI
						r_wfi <= 1'b1;
						r_stage <= 3'h0;
					end
					16'b000_001_000_000_0000: begin // RETI
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h5;
						end
						if (r_stage == 5 && i_wb_ready) begin
							r_stage <= 3'h6;
						end
						if (r_stage == 6 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b000_???_001_000_0000: begin // SWI
						r_interrupt_internal <= 1'b1;
						r_stage <= 3'h0;
					end
					16'b001_???_001_000_0000: begin // PUSH
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b???_000_010_000_0000: begin // POP
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b???_000_??1_111_0000: begin // BL
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h5;
						end
						if (r_stage == 5 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b???_???_???_???_1100: begin // SB
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b???_???_???_???_1101: begin // LB
						if (r_stage == 3) begin
							r_stage <= 3'h4;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					16'b???_???_???_???_1111: begin //BL
						if (r_stage == 3) begin
							if (w_cond) r_stage <= 3'h4;
							else r_stage <= 3'h0;
						end
						if (r_stage == 4 && i_wb_ready) begin
							r_stage <= 3'h5;
						end
						if (r_stage == 5 && i_wb_ready) begin
							r_stage <= 3'h0;
						end
					end
					default: r_stage <= 3'h0;
				endcase
			end

			if (w_illegal_instruction) begin
				r_interrupt_internal <= 1'b1;
			end

			if (r_stage == 0 && r_interrupt_stage == 0 && w_interrupt) begin
				r_interrupt_stage <= 3'h1;
				if (~r_interrupt_internal) begin
					// acknowledge interrupt
					or_interrupt_ack <= 1'b1;
				end
				// remove WFI
				r_wfi <= 1'b0;
				// remove internal interrrupt flag
				r_interrupt_internal <= 1'b0;
			end else if (r_interrupt_stage == 1 && i_wb_ready) begin
				r_interrupt_stage <= 3'h2;
				// unacknowledge interrupt
				or_interrupt_ack <= 1'b0;
			end else if (r_interrupt_stage == 2 && i_wb_ready) begin
				r_interrupt_stage <= 3'h3;
			end else if (r_interrupt_stage == 3 && i_wb_ready) begin
				r_interrupt_stage <= 3'h4;
			end else if (r_interrupt_stage == 4 && i_wb_ready) begin
				r_interrupt_stage <= 3'h5;
			end else if (r_interrupt_stage == 5 && i_wb_ready) begin
				r_interrupt_stage <= 3'h0;
			end
		end
	end


endmodule