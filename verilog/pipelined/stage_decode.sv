`timescale 1ns / 1ps

module stage_decode (
	input  logic		i_clk,
	input  logic		i_rst,

	// Pipeline interface (from IF stage / IF/ID register)
	input  logic [15:0]	i_instr,
	input  logic [15:1]	i_pc,
	input  logic [15:1]	i_npc,

	// stall interface
	input  logic		i_stall,
	output logic		o_stall,

	// Interrupt IO
	input  logic		i_int,
	input  logic [6:0]	i_int_id,
	output logic [6:0]	o_int_id,
	output logic		o_int_ack,

	// Instruction Fetch stage IO
	output logic		o_if_stall,

	// Instruction Decode stage IO
	output logic [4:0]	o_id_flags,
	output logic [7:0]	o_id_a,
	output logic [7:0]	o_id_b,
	output logic [15:0]	o_id_adata,
	output logic [15:0]	o_id_imm,

	// EXecute stage IO
	input  logic [4:0]	i_ex_flags,
	input  logic [15:0]	i_ex_alu_result,
	input  logic [15:0]	i_ex_adata,

	input  logic		i_ex_rf_we,
	input  logic [2:0]	i_ex_rf_wsel,
	input  logic		i_ex_cf_we,
	input  logic		i_ex_cf_flags_we,
	input  logic		i_ex_cf_sp_we,
	input  logic [2:0]	i_ex_cf_wsel,
	input  logic		i_ex_mem_re,
	
	output logic [1:0]	o_ex_alu_a_sel,
	output logic		o_ex_alu_b_sel,
	output logic		o_ex_wdata_sel,
	output logic		o_ex_adata_sel,
	output logic [3:0]	o_ex_alu_op,
	output logic		o_ex_adr_sel,

	// MEMory Stage IO
	input  logic [4:0]	i_mem_flags,
	input  logic [7:0]	i_mem_rdata,
	input  logic [15:0]	i_mem_adata,

	input  logic		i_mem_rf_we,
	input  logic [2:0]	i_mem_rf_wsel,
	input  logic		i_mem_cf_we,
	input  logic		i_mem_cf_flags_we,
	input  logic		i_mem_cf_sp_we,
	input  logic [2:0]	i_mem_cf_wsel,
	
	output logic		o_mem_re,
	output logic		o_mem_we,

	// WriteBack stage IO
	input  logic [7:0]	i_wb_rdata,
	input  logic [15:0]	i_wb_adata,

	input  logic		i_wb_rf_we,
	input  logic [2:0]	i_wb_rf_wsel,
	input  logic		i_wb_cf_we,
	input  logic		i_wb_cf_flags_we,
	input  logic		i_wb_cf_sp_we,
	input  logic [2:0]	i_wb_cf_wsel,
	input  logic [4:0]	i_wb_flags,

	output logic		o_wb_rf_we,
	output logic [2:0]	o_wb_rf_wsel,
	output logic		o_wb_cf_we,
	output logic		o_wb_cf_flags_we,
	output logic		o_wb_cf_sp_we,
	output logic [2:0]	o_wb_cf_wsel,

	output logic		o_wb_pc_lwe,
	output logic		o_wb_pc_uwe,
	output logic		o_wb_pc_we,

	// ifid hold
	output logic		o_ifid_hold
);

	// Internal wires
	// Interrupt wires
	logic [6:0]	w_cu_int_id;
	logic		w_int_id_we;
	logic		w_int_id_sel;
	logic [6:0]	r_int_id;

	// Register File wires
	logic [7:0]	w_rf_rdata1;
	logic [7:0]	w_rf_rdata2;
	logic [15:0]	w_rf_adata;

	// CSR File wires
	logic [7:0]	w_cf_rdata;
	logic [4:0]	w_cf_flags;
	logic [15:0]	w_cf_sp;

	// Misc
	logic [4:0]	w_id_a_sel;
	logic [2:0]	w_id_b_sel;
	logic [2:0]	w_id_ig_sel;
	logic [2:0]	w_id_adata_sel;
	logic [15:0]	w_id_adata;
	logic [3:0]	w_id_adata_selp;
	logic [2:0]	w_id_flags_sel;
	logic [4:0]	w_id_flags;

	logic [1:0]	w_cu_stage;
	logic [2:0]	w_cu_int_stage;
	logic		w_cu_int;
	logic		w_cu_swi;

	// flags_we mux
	logic		w_cu_flags_sel;

	// Forwarding Unit instance
	forwarding_unit u_fwd (
		.i_instr(i_instr),

		// Pipeline feedback inputs
		.i_ex_rf_we(i_ex_rf_we),
		.i_ex_rf_wsel(i_ex_rf_wsel),
		.i_mem_rf_we(i_mem_rf_we),
		.i_mem_rf_wsel(i_mem_rf_wsel),
		.i_wb_rf_we(i_wb_rf_we),
		.i_wb_rf_wsel(i_wb_rf_wsel),

		.i_ex_cf_we(i_ex_cf_we),
		.i_ex_cf_wsel(i_ex_cf_wsel),
		.i_mem_cf_we(i_mem_cf_we),
		.i_mem_cf_wsel(i_mem_cf_wsel),
		.i_wb_cf_we(i_wb_cf_we),
		.i_wb_cf_wsel(i_wb_cf_wsel),

		.i_ex_cf_sp_we(i_ex_cf_sp_we),
		.i_mem_cf_sp_we(i_mem_cf_sp_we),
		.i_wb_cf_sp_we(i_wb_cf_sp_we),

		.i_ex_cf_flags_we(i_ex_cf_flags_we),
		.i_mem_cf_flags_we(i_mem_cf_flags_we),
		.i_wb_cf_flags_we(i_wb_cf_flags_we),

		.i_ex_mem_re(i_ex_mem_re),

		// Forwarding selectors
		.o_id_a_sel(w_id_a_sel),
		.o_id_b_sel(w_id_b_sel),
		.o_id_adata_sel(w_id_adata_sel),
		.o_id_adata_selp(w_id_adata_selp),
		.o_id_flags_sel(w_id_flags_sel),

		.o_stall(o_stall),

		.i_stage(w_cu_stage),
		.i_int_stage(w_cu_int_stage),
		.i_int(w_cu_int),
		.i_swi(w_cu_swi)
	);


	// Control Unit instance
	control_unit u_cu (
		.i_clk(i_clk),
		.i_rst(i_rst),
		.i_instr(i_instr),
		.i_flags(w_id_flags),

		.i_int(i_int),
		.o_int_id(w_cu_int_id),
		.o_int_ack(o_int_ack),
		.o_int_id_we(w_int_id_we),
		.o_int_id_sel(w_int_id_sel),
		.i_id_rdata(w_rf_rdata1[6:0]),

		// Pipeline stall signals
		.i_stall(i_stall),

		// Instruction Fetch stage IO
		.o_if_stall(o_if_stall),

		// Decode stage selectors
		.o_id_ig_sel(w_id_ig_sel),
		.i_id_a(o_id_a),
		.o_cu_flags_sel(w_cu_flags_sel),

		// Execute stage IO
		.o_ex_alu_a_sel(o_ex_alu_a_sel),
		.o_ex_alu_b_sel(o_ex_alu_b_sel),
		.o_ex_wdata_sel(o_ex_wdata_sel),
		.o_ex_adata_sel(o_ex_adata_sel),
		.o_ex_alu_op(o_ex_alu_op),
		.o_ex_adr_sel(o_ex_adr_sel),

		// Memory stage IO
		.o_mem_re(o_mem_re),
		.o_mem_we(o_mem_we),

		// WriteBack stage IO
		.o_wb_rf_we(o_wb_rf_we),
		.o_wb_cf_we(o_wb_cf_we),
		.o_wb_cf_flags_we(o_wb_cf_flags_we),
		.o_wb_cf_sp_we(o_wb_cf_sp_we),
		.o_wb_pc_lwe(o_wb_pc_lwe),
		.o_wb_pc_uwe(o_wb_pc_uwe),
		.o_wb_pc_we(o_wb_pc_we),

		// stage
		.o_stage(w_cu_stage),
		.o_int_stage(w_cu_int_stage),
		.o_int(w_cu_int),
		.o_hold(o_ifid_hold),
		.o_swi(w_cu_swi)
	);

	always_ff @(posedge i_clk) begin
		if (w_int_id_we && ~i_stall) begin
			case (w_int_id_sel)
				1'b0: r_int_id <= i_int_id;
				1'b1: r_int_id <= w_cu_int_id;
			endcase
		end
	end

	// General Purpose Register File instance
	register_file u_rf (
		.i_clk(i_clk),
		.i_we(i_wb_rf_we),
		.i_rsel1(i_instr[12:10]),
		.i_rsel2(i_instr[9:7]),
		.i_wsel(i_wb_rf_wsel),
		.i_wdata(i_wb_rdata),
		.o_rdata1(w_rf_rdata1),
		.o_rdata2(w_rf_rdata2),
		.o_adata(w_rf_adata)
	);

	// CSR File instance
	csr_file u_cf (
		.i_clk(i_clk),
		.i_rst(i_rst),
		.i_we(i_wb_cf_we),
		.i_flags_we(i_wb_cf_flags_we),
		.i_sp_we(i_wb_cf_sp_we),
		.i_rsel(i_instr[12:10]),
		.i_wsel(i_wb_cf_wsel),
		.i_flags(i_wb_flags),
		.i_wdata(i_wb_rdata),
		.i_sp(i_wb_adata),
		.o_rdata(w_cf_rdata),
		.o_flags(w_cf_flags),
		.o_sp(w_cf_sp)
	);

	// Immediate Generator instance
	immediate_gen u_ig (
		.i_instr(i_instr[15:4]),
		.i_sel(w_id_ig_sel),
		.o_immediate(o_id_imm)
	);

	always_comb begin
		o_wb_rf_wsel = i_instr[15:13];
		o_wb_cf_wsel = w_cu_flags_sel ? 3'h0 : i_instr[15:13];

		case (w_id_flags_sel)
			3'h0: w_id_flags = w_cf_flags;
			3'h1: w_id_flags = i_ex_flags;
			3'h2: w_id_flags = i_mem_flags;
			3'h3: w_id_flags = i_wb_flags;
			3'h4: w_id_flags = {i_ex_alu_result[7], i_ex_alu_result[3:0]};
			3'h5: w_id_flags = {i_mem_rdata[7], i_mem_rdata[3:0]};
			3'h6: w_id_flags = {i_wb_rdata[7], i_wb_rdata[3:0]};
			default: w_id_flags = 5'hx;
		endcase

		o_id_flags = w_id_flags;

		case (w_id_a_sel)
			5'h00: o_id_a = w_rf_rdata1;
			5'h01: o_id_a = w_cf_rdata;
			5'h02: o_id_a = i_ex_alu_result[7:0];
			5'h03: o_id_a = i_mem_rdata;
			5'h04: o_id_a = i_wb_rdata;
			5'h05: o_id_a = i_ex_adata[7:0];
			5'h06: o_id_a = i_mem_adata[7:0];
			5'h07: o_id_a = i_wb_adata[7:0];
			5'h08: o_id_a = i_ex_adata[15:8];
			5'h09: o_id_a = i_mem_adata[15:8];
			5'h0A: o_id_a = i_wb_adata[15:8];
			5'h10: o_id_a = {w_id_flags[4], 3'h0, w_id_flags[3:0]};
			5'h11: o_id_a = {i_pc[7:1], 1'b0};
			5'h12: o_id_a = i_pc[15:8];
			5'h13: o_id_a = {i_npc[7:1], 1'b0};
			5'h14: o_id_a = i_npc[15:8];
			default: o_id_a = 8'hx;
		endcase
		case (w_id_b_sel)
			3'h0: o_id_b = w_rf_rdata2;
			3'h1: o_id_b = i_ex_alu_result[7:0];
			3'h2: o_id_b = i_mem_rdata;
			3'h3: o_id_b = i_wb_rdata;
			default: o_id_b = 8'hx;
		endcase
		case (w_id_adata_sel)
			3'h0: w_id_adata = w_rf_adata;
			3'h1: w_id_adata = w_cf_sp;
			3'h2: w_id_adata = i_ex_adata;
			3'h3: w_id_adata = i_mem_adata;
			3'h4: w_id_adata = i_wb_adata;
			3'h5: w_id_adata = {8'hFF, r_int_id, 1'b0};
			3'h6: w_id_adata = {8'hFF, r_int_id, 1'b1};
			default: o_id_adata = 16'hx;
		endcase
		case (w_id_adata_selp)
			4'h0: o_id_adata = w_id_adata;
			4'h1: o_id_adata = {w_id_adata[15:8], i_ex_alu_result[7:0]};
			4'h2: o_id_adata = {w_id_adata[15:8], i_mem_rdata};
			4'h3: o_id_adata = {w_id_adata[15:8], i_wb_rdata};
			4'h4: o_id_adata = {i_ex_alu_result[7:0], w_id_adata[7:0]};
			4'h5: o_id_adata = {i_mem_rdata, w_id_adata[7:0]};
			4'h6: o_id_adata = {i_wb_rdata, w_id_adata[7:0]};
			4'h7: o_id_adata = {i_ex_alu_result[7:0], i_mem_rdata};
			4'h8: o_id_adata = {i_ex_alu_result[7:0], i_wb_rdata};
			4'h9: o_id_adata = {i_mem_rdata, i_ex_alu_result[7:0]};
			4'hA: o_id_adata = {i_mem_rdata, i_wb_rdata};
			4'hB: o_id_adata = {i_wb_rdata, i_ex_alu_result[7:0]};
			4'hC: o_id_adata = {i_wb_rdata, i_mem_rdata};
			default: o_id_adata = 16'hx;
		endcase
	end

endmodule
