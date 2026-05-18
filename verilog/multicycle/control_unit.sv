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
	output logic [2:0]  o_rf_rsel1,
	output logic [2:0]  o_rf_rsel2,
	
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
	output logic [1:0]  o_ctr_address_sel,
	output logic        o_ctr_address_we,
	output logic        o_ctr_data_we,
	output logic        o_ctr_instruction_lwe,
	output logic        o_ctr_instruction_uwe,
	output logic [1:0]  o_ctr_cf_wdata_sel,

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
		o_rf_rsel1 = i_instruction[12:10];
		o_rf_rsel2 = i_instruction[9:7];
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
		o_ctr_address_sel = 2'h0;
		o_ctr_address_we = 1'b0;
		o_ctr_data_we = 1'b0;
		o_ctr_instruction_lwe = 1'h0;
		o_ctr_instruction_uwe = 1'h0;
		o_interrupt_id = 7'h0;
		o_interrupt_id_we = 1'b0;
		o_interrupt_id_sel = 1'b0;

		if (r_stage == 0) begin // Fetch instruction byte 1
			// request lower byte of instruction
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_ctr_address_sel = 2'h2;
			o_ctr_address_we = 1'b1;
		end
		if (r_stage == 1 && i_wb_ready) begin
			// request upper byte of instruction and write lower
			o_wb_req = 1'b1;
			o_wb_lock = 1'b1;
			o_ctr_address_sel = 2'h3;
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
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 2'h2;
						o_ctr_address_we = 1'b1;
					end
					if (r_stage == 4 && i_wb_ready) begin
						o_wb_req = 1'b1;
						o_wb_lock = 1'b1;
						o_ctr_address_sel = 2'h3;
						o_ctr_address_we = 1'b1;
						o_ctr_instruction_lwe = 1'b1;
					end
				end
				16'b010_000_000_000_0000: ; // WFI
				16'b000_001_000_000_0000: begin // RETI
				end
				16'b???_010_000_000_0000: begin // INCC
					// select operand
					o_rf_rsel1 = i_instruction[15:13];
					// select operation and inputs
					o_alu_op = 4'h1;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'b1;
					o_ig_sel = 3'h5;
					// write register
					o_rf_we = 1'b1;
					// write flags
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				16'b???_011_000_000_0000: begin // DECB
					// select operand
					o_rf_rsel1 = i_instruction[15:13];
					// select operation and inputs
					o_alu_op = 4'h3;
					o_alu_a_sel = 2'h0;
					o_alu_b_sel = 1'b1;
					o_ig_sel = 3'h5;
					// write register
					o_rf_we = 1'b1;
					// write flags
					o_cf_wsel = 3'h0;
					o_cf_we = 1'b1;
				end
				default: w_illegal_instruction = 1'b1;
			endcase
		end
	end

	always_ff @(posedge i_clk) begin
		if (i_rst) begin
			r_stage <= 3'b0;
			r_interrupt_stage <= 3'b0;
			r_interrupt_internal <= 1'b0;
			r_wfi <= 1'b0;
		end else begin
			if (r_stage == 0 && ~r_wfi) begin
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
					end
					default: r_stage <= 3'h0;
				endcase
			end
		end
	end


endmodule