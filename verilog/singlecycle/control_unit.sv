module control_unit(
	// Clock and Reset
	input logic		i_clk,
	input logic		i_rst_n,

	// Decoder inputs
	input  logic [15:0]	i_instr,
	input  logic [2:0]	i_rsel2,
	input  logic [2:0]	i_rsel1,
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
	output logic [2:0]	o_mem_wdata_sel,
	output logic		o_mem_we,
	output logic [2:0]	o_cf_wdata_sel,

	// Interrupt Interface
	input  logic		i_interrupt,
	output logic [6:0]	o_interrupt_id,
	output logic		or_interrupt_ack,
	output logic		o_interrupt_id_we,
	output logic		o_interrupt_id_sel,
	input logic [6:0]	i_rdata
);

	// instruction stage flip flops
	logic [1:0]	r_stage;

	logic 		w_cond;

	// interrupt stage flip flops and wires
	logic 		w_interrupt;
	logic [2:0]	r_interrupt_stage;
	logic		r_interrupt_internal;

	// wait for interrupt flip flop
	logic 		r_wfi;

	// illegal instruction signal
	logic		w_illegal_instruction;


	always_comb begin
		w_interrupt = (i_interrupt && i_flags[7]) || r_interrupt_internal;
		w_illegal_instruction = 1'b0;

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
		o_mem_wdata_sel = 3'h0;
		o_mem_we = 1'b0;
		o_cf_wdata_sel = 3'h0;
		o_interrupt_id_we = 1'b0;
		o_interrupt_id_sel = 1'b0;
		o_interrupt_id = 7'b0;
		
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

		if (r_wfi) begin
			o_pc_we = 1'b0;
		end

		if (r_interrupt_stage == 0 && ~w_interrupt && ~r_wfi) begin
		casez (i_instr)
			16'b000_000_000_000_0000: ; // NOP
			16'b001_000_000_000_0000: begin // RET
				// SP increment
				o_cf_wsp = 1'b1;
				o_sp_sel = 2'h1;
				o_agu_sel = 3'h4;

				if (r_stage == 0) begin
					o_pc_rwe = 1'b1;
					o_pc_we = 1'b0;
				end
				if (r_stage == 1) begin
					o_pc_sel = 2'h3;
					o_pc_we = 1'b1;
				end
			end
			16'b010_000_000_000_0000: ; // WFI
			16'b000_001_000_000_0000: begin // RETI
				// SP increment
				o_cf_wsp = 1'b1;
				o_sp_sel = 2'h1;
				o_agu_sel = 3'h4;

				if (r_stage == 0) begin
					o_pc_we = 1'b0;
					// Load flags from memory
					o_cf_rsel = 3'h0;
					o_cf_we = 1'b1;
					o_cf_wdata_sel = 3'h4;
				end
				if (r_stage == 1) begin
					// write PC lower
					o_pc_rwe = 1'b1;
					o_pc_we = 1'b0;
				end
				if (r_stage == 2) begin
					// load PC upper
					o_pc_sel = 2'h3;
					o_pc_we = 1'b1;
				end
			end
			16'b???_010_000_000_0000: begin // INCC
				o_alu_op = 4'h1;
				o_rf_we = 1'b1;
				o_rf_rsel1 = i_wsel;
				o_cf_we = 1'b1;
				o_ig_sel = 3'h7;
				o_alu_b_sel = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_011_000_000_0000: begin // DECB
				o_alu_op = 4'h3;
				o_rf_we = 1'b1;
				o_rf_rsel1 = i_wsel;
				o_cf_we = 1'b1;
				o_ig_sel = 3'h7;
				o_alu_b_sel = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_101_000_000_0000: begin // SWI
				o_rf_rsel1 = i_wsel;
				o_interrupt_id = i_rdata;
				o_interrupt_id_sel = 1'b1;
				o_interrupt_id_we = 1'b1;
			end
			16'b???_110_000_000_0000: begin // POP
				o_rf_we = 1'b1;
				o_cf_wsp = 1'b1;
				o_cf_wdata_sel = 3'h2;
				o_sp_sel = 2'h1; 
				o_agu_sel = 3'h4;
				o_rf_wdata_sel = 2'h2;
			end
			16'b???_111_000_000_0000: begin // PUSH
				o_rf_rsel1 = i_wsel;
				o_cf_wsp = 1'b1;
				o_cf_wdata_sel = 3'h2;
				o_sp_sel = 2'h2; 
				o_agu_sel = 3'h3;
				o_mem_we = 1'b1;
			end
			16'b???_???_000_001_0000: begin // MOV csr->gpr
				// select csr
				o_cf_rsel = i_rsel1;

				// write csr output to gpr
				o_rf_we = 1'b1;
				o_rf_wdata_sel = 2'h1;

				// handle flags
				o_cf_wdata_sel = 3'h2;
				o_cf_we = 1'b1;
			end
			16'b???_???_001_001_0000: begin // MOV gpr->csr
				o_cf_we = 1'b1;
				o_cf_wsel = i_wsel;
			end
			16'b???_???_010_001_0000: begin // CMP
				// operands
				o_rf_rsel2 = i_wsel;
				// ALU op
				o_alu_op = 4'h2;
				// flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h2;
			end
			16'b???_???_011_001_0000: begin // CMA
				// operands
				o_rf_rsel2 = i_wsel;
				// ALU op
				o_alu_op = 4'h4;
				// flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h2;
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
				o_agu_sel = 3'h3;
				o_sp_sel = 2'h2;
				if (w_cond && r_stage == 0) begin
					// store PC upper
					o_mem_wdata_sel = 3'h3;
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					// disable PC update
					o_pc_we = 1'b0;
				end
				if (w_cond && r_stage == 1) begin
					// store PC lower
					o_mem_wdata_sel = 3'h2;
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					// load new PC
					o_rf_rsel2 = i_rsel1;
					o_pc_sel = 2'h1;
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
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_001_0001: begin // SBB
				// subtract with borrow
				o_alu_op = 4'h3;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_010_0001: begin // ADD
				// add
				o_alu_op = 4'h0;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_011_0001: begin // ADC
				// add with carry
				o_alu_op = 4'h1;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_100_0001: begin // AND
				// and
				o_alu_op = 4'h4;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_101_0001: begin // OR
				// or
				o_alu_op = 4'h5;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_110_0001: begin // NOR
				// nor
				o_alu_op = 4'h6;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_111_0001: begin // XOR
				// xor
				o_alu_op = 4'h7;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_000_0010: begin // SLL
				// shift left logical
				o_alu_op = 4'h8;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_001_0010: begin // SRL
				// shift right logical
				o_alu_op = 4'h9;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_010_0010: begin // SRA
				// shift right arithmetic
				o_alu_op = 4'hA;
				// store result in register
				o_rf_wdata_sel = 2'h0;
				o_rf_we = 1'b1;
				// store flags
				o_cf_we = 1'b1;
				o_cf_wdata_sel = 3'h1;
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
				o_cf_wdata_sel = 3'h1;
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
				o_cf_wdata_sel = 3'h1;
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
				o_cf_wdata_sel = 3'h1;
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
				o_cf_wdata_sel = 3'h2;
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
				o_cf_wdata_sel = 3'h1;
			end
			16'b???_???_???_???_1100: begin // SB
				// operand A
				o_rf_rsel1 = i_wsel;
				// operand B
				o_rf_rsel2 = i_rsel1;
				// output A to mem
				o_mem_wdata_sel = 3'h0;
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
				o_cf_wdata_sel = 3'h2;
			end
			16'b???_???_???_???_1110: begin // B
				if (w_cond) begin
					// immediate operand
					o_ig_sel = 3'h4;
					// load new PC
					o_pc_sel = 2'h2;
					o_pc_we = 1'b1;
				end
			end
			16'b???_???_???_???_1111: begin // BL
				o_agu_sel = 3'h3;
				o_sp_sel = 2'h2;
				if (w_cond && r_stage == 0) begin
					// select PC upper
					o_mem_wdata_sel = 3'h3;
					// store PC upper
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					// disable PC update
					o_pc_we = 1'b0;
				end
				if (w_cond && r_stage == 1) begin
					// select PC lower
					o_mem_wdata_sel = 3'h2;
					// store PC lower
					o_mem_we = 1'b1;
					// SP decrement
					o_cf_wsp = 1'b1;
					// load new PC
					o_ig_sel = 3'h4;
					o_pc_sel = 2'h2;
					o_pc_we = 1'b1;
				end
			end
			default: begin // no instruction
				o_interrupt_id = 7'h2;
				o_interrupt_id_sel = 1'b1;
				o_interrupt_id_we = 1'b1;
				w_illegal_instruction = 1'b1;
			end
		endcase
		end

		// interrupt handling
		if (r_stage == 0 && r_interrupt_stage == 0 && w_interrupt) begin
			// disable PC update
			o_pc_we = 1'b0;
			// store PC upper
			o_mem_wdata_sel = 3'h6;
			o_mem_we = 1'b1;
			o_agu_sel = 3'h3;
			// SP decrement
			o_sp_sel = 2'h2;
			o_cf_wsp = 1'b1;
			if (~r_interrupt_internal) begin
				// write external interrupt id to register
				o_interrupt_id_sel = 1'b0;
				o_interrupt_id_we = 1'b1;
			end
		end else if (r_interrupt_stage == 1) begin
			// disable PC update
			o_pc_we = 1'b0;
			// store PC upper
			o_mem_wdata_sel = 3'h5;
			o_mem_we = 1'b1;
			o_agu_sel = 3'h3;
			// SP decrement
			o_sp_sel = 2'h2;
			o_cf_wsp = 1'b1;
		end else if (r_interrupt_stage == 2) begin
			// disable PC update
			o_pc_we = 1'b0;
			// store flags
			o_mem_wdata_sel = 3'h4;
			o_mem_we = 1'b1;
			o_agu_sel = 3'h3;
			// SP decrement
			o_sp_sel = 2'h2;
			o_cf_wsp = 1'b1;
			// disable interrupts
			o_cf_rsel = 3'h0;
			o_cf_wdata_sel = 3'h3;
			o_cf_we = 1'b1;
		end else if (r_interrupt_stage == 3) begin
			// disable PC update
			o_pc_we = 1'b0;
			// read new PC lower
			o_agu_sel = 3'h6;
			// write new PC lower to temp register
			o_pc_rwe = 1'b1;
		end else if (r_interrupt_stage == 4) begin
			// enable PC update
			o_pc_we = 1'b1;
			// read new PC upper
			o_agu_sel = 3'h7;
			// write new PC
			o_pc_sel = 2'h3;
		end
	end

	always_ff @(posedge i_clk or negedge i_rst_n) begin
		if (!i_rst_n) begin
			r_stage <= 2'h0;
			r_interrupt_stage <= 3'h0;
			or_interrupt_ack <= 1'b0;
			r_wfi <= 1'b0;
			r_interrupt_internal <= 1'b0;
		end


		if (r_interrupt_stage == 0 && ~w_interrupt && ~r_wfi) begin
		casez (i_instr)
			16'b001_000_000_000_0000: begin // RET
				if (r_stage == 0 && r_interrupt_stage == 0 && ~w_interrupt) r_stage <= 2'h1;
				else if (r_stage == 1) r_stage <= 2'h0;
			end
			16'b010_000_000_000_0000: begin // WFI
				r_wfi <= 1'b1;
			end
			16'b000_001_000_000_0000: begin //RETI
				if (r_stage == 0 && r_interrupt_stage == 0 && ~w_interrupt) r_stage <= 2'h1;
				else if (r_stage == 1) r_stage <= 2'h2;
				else if (r_stage == 2) r_stage <= 2'h0;
			end
			16'b???_101_000_000_0000: begin // SWI
				r_interrupt_internal <= 1'b1;
			end
			16'b???_??1_000_111_0000: begin // BL
				if (r_stage == 0 && w_cond && r_interrupt_stage == 0 && ~w_interrupt) r_stage <= 2'h1;
				else if (r_stage == 1) r_stage <= 2'h0;
			end
			16'b???_???_???_???_1111: begin // BL
				if (r_stage == 0 && w_cond && r_interrupt_stage == 0 && ~w_interrupt) r_stage <= 2'h1;
				else if (r_stage == 1) r_stage <= 2'h0;
			end
			default: r_stage <= 2'b00;
		endcase
		end

		if (w_illegal_instruction) begin
			r_interrupt_internal <= 1'b1;
		end

		if (r_stage == 0 && r_interrupt_stage == 0 && w_interrupt) begin
			// advance interrupt stage
			r_interrupt_stage <= 3'h1;
			if (~r_interrupt_internal) begin
				// acknowledge interrupt
				or_interrupt_ack <= 1'b1;
			end
			// remove WFI
			r_wfi <= 1'b0;
			// remove internal interrupt flag
			r_interrupt_internal <= 1'b0;
		end else if (r_interrupt_stage == 1) begin
			r_interrupt_stage <= 3'h2;
			// unacknowledge interrupt
			or_interrupt_ack <= 1'b0;
		end else if (r_interrupt_stage == 2) begin
			r_interrupt_stage <= 3'h3;
		end else if (r_interrupt_stage == 3) begin
			r_interrupt_stage <= 3'h4;
		end else if (r_interrupt_stage == 4) begin
			r_interrupt_stage <= 3'h0;
		end
	end

endmodule