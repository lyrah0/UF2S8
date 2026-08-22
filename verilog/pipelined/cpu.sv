`timescale 1ps/1ps

module cpu (
	input  logic		clk_i,
	input  logic		rst_i,
	
	// Memory interface
	input  logic		imem_stall_i,
	input  logic [15:0]	imem_dat_i,
	output logic [14:0]	imem_adr_o,
	input  logic		dmem_stall_i,
	input  logic [7:0]	dmem_dat_i,
	output logic		dmem_re_o,
	output logic		dmem_we_o,
	output logic [7:0]	dmem_dat_o,
	output logic [15:0]	dmem_adr_o,

	// interrupt signals
	input  logic		i_int,
	input  logic [6:0]	i_int_id,
	output logic		o_int_ack
);
	// Instruction Fetch Wires
	logic [15:1]	w_if_pc;
	logic [15:1]	w_if_npc;
	logic [15:0]	w_if_inst;
	logic		w_if_stall;

	// Instruction Decode wires
	logic		w_id_stall;
	logic [6:0] 	w_id_int_id;
	logic [4:0]	w_id_flags;
	logic [7:0]	w_id_a;
	logic [7:0]	w_id_b;
	logic [15:0]	w_id_adata;
	logic [15:0]	w_id_imm;
	logic		w_ex_adr_sel;
	logic		w_ex_wdata_sel;
	logic		w_ex_adata_sel;
	logic		w_wb_rf_we;
	logic [2:0] 	w_wb_rf_wsel;
	logic		w_wb_cf_we;
	logic		w_wb_cf_flags_we;
	logic		w_wb_cf_sp_we;
	logic [2:0]	w_wb_cf_wsel;

	// Execute Wires
	logic [15:0]	w_ex_alu_a;
	logic [15:0]	w_ex_alu_b;
	logic [15:0]	w_ex_alu_imm;
	logic [1:0]	w_ex_alu_a_sel;
	logic		w_ex_alu_b_sel;
	logic [3:0]	w_ex_alu_op;
	logic		w_ex_alu_carry;
	logic [15:0]	w_ex_alu_result;
	logic [3:0]	w_ex_alu_flags;
	logic [15:0]	w_ex_adata;
	logic [15:0]	w_ex_adr;
	logic		w_ex_rf_we;
	logic [2:0]	w_ex_rf_wsel;
	logic		w_ex_cf_we;
	logic		w_ex_cf_flags_we;
	logic		w_ex_cf_sp_we;
	logic [2:0]	w_ex_cf_wsel;

	// Memory wires
	logic [15:0]	w_mem_addr;
	logic [7:0]	w_mem_data;
	logic		w_mem_re;
	logic		w_mem_we;
	logic [7:0]	w_mem_rdata;
	logic [3:0]	w_mem_flags;
	logic [15:0]	w_mem_adata;
	logic		w_mem_rf_we;
	logic [2:0]	w_mem_rf_wsel;
	logic		w_mem_cf_we;
	logic		w_mem_cf_flags_we;
	logic		w_mem_cf_sp_we;
	logic [2:0]	w_mem_cf_wsel;

	// WriteBack wires
	logic		w_wb_pc_lwe;
	logic		w_wb_pc_uwe;
	logic		w_wb_pc_we;
	logic [14:0]	w_wb_pc;
	logic [7:0]	w_wb_rdata;
	logic [15:0]	w_wb_adata;

	// Pipeline registers
	// IF/ID
	logic [14:0]	r_ifid_pc;
	logic [14:0]	r_ifid_npc;
	logic [15:0]	r_ifid_inst;

	// ID/EX
	logic [14:0]	r_idex_npc;
	logic [4:0]	r_idex_flags;
	logic [7:0]	r_idex_a;
	logic [7:0]	r_idex_b;
	logic [15:0]	r_idex_adata;
	logic [15:0]	r_idex_imm;
	logic [1:0]	r_idex_alu_a_sel;
	logic		r_idex_alu_b_sel;
	logic [3:0]	r_idex_alu_op;
	logic		r_idex_adr_sel;
	logic		r_idex_wdata_sel;
	logic		r_idex_mem_re;
	logic		r_idex_mem_we;
	logic		r_idex_wb_rf_we;
	logic [2:0]	r_idex_wb_rf_wsel;
	logic		r_idex_wb_cf_we;
	logic		r_idex_wb_cf_flags_we;
	logic		r_idex_wb_cf_sp_we;
	logic [2:0]	r_idex_wb_cf_wsel;
	logic		r_idex_adata_sel;
	logic		r_idex_wb_pc_lwe;
	logic		r_idex_wb_pc_uwe;
	logic		r_idex_wb_pc_we;

	// EX/MEM
	logic [4:0]	r_exmem_flags;
	logic [15:0]	r_exmem_adr;
	logic [7:0]	r_exmem_wdata;
	logic [15:0]	r_exmem_adata;
	logic		r_exmem_mem_re;
	logic		r_exmem_mem_we;
	logic		r_exmem_wb_rf_we;
	logic [2:0]	r_exmem_wb_rf_wsel;
	logic		r_exmem_wb_cf_we;
	logic		r_exmem_wb_cf_flags_we;
	logic		r_exmem_wb_cf_sp_we;
	logic [2:0]	r_exmem_wb_cf_wsel;
	logic		r_exmem_wb_pc_lwe;
	logic		r_exmem_wb_pc_uwe;
	logic		r_exmem_wb_pc_we;

	// MEM/WB
	logic [4:0]	r_memwb_flags;
	logic [7:0]	r_memwb_rdata;
	logic [15:0]	r_memwb_adata;
	logic		r_memwb_rf_we;
	logic [2:0]	r_memwb_rf_wsel;
	logic		r_memwb_cf_we;
	logic		r_memwb_cf_flags_we;
	logic		r_memwb_cf_sp_we;
	logic [2:0]	r_memwb_cf_wsel;
	logic		r_memwb_pc_lwe;
	logic		r_memwb_pc_uwe;
	logic		r_memwb_pc_we;

	// IF/ID hold wire
	logic		w_ifid_hold;

	// Fetch stage
	stage_fetch u_if(
		.i_clk(clk_i),
		.i_rst(rst_i),
		.i_stall(imem_stall_i || dmem_stall_i || w_id_stall || w_if_stall),
		.i_pc_lwe(r_memwb_pc_lwe),
		.i_pc_uwe(r_memwb_pc_uwe),
		.i_pc_we(r_memwb_pc_we),
		.i_data(r_memwb_rdata),
		.i_adata(r_memwb_adata[15:1]),
		.o_pc(w_if_pc),
		.o_npc(w_if_npc),
		.o_inst(w_if_inst),
		.imem_stall_i(imem_stall_i),
		.imem_dat_i(imem_dat_i),
		.imem_adr_o(imem_adr_o)
	);

	// IF/ID Register
	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			r_ifid_inst	<= 16'b0;
			r_ifid_pc	<= 15'b0;
			r_ifid_npc	<= 15'b0;
		end else if (w_if_stall && !w_ifid_hold) begin
			r_ifid_inst	<= 16'b0;
		end else if (!(dmem_stall_i || w_id_stall || w_ifid_hold)) begin
			r_ifid_pc	<= w_if_pc;
			r_ifid_npc	<= w_if_npc;
			r_ifid_inst	<= w_if_inst;
		end
	end

	// Decode stage
	stage_decode u_id (
		.i_clk(clk_i),
		.i_rst(rst_i),

		// Pipeline interface
		.i_instr(r_ifid_inst),
		.i_pc(r_ifid_pc),
		.i_npc(r_ifid_npc),

		// Stall interface
		.i_stall(dmem_stall_i || w_id_stall),
		.o_stall(w_id_stall),

		// Interrupt IO
		.i_int(i_int),
		.i_int_id(i_int_id),
		.o_int_id(w_id_int_id),
		.o_int_ack(o_int_ack),

		// Fetch outputs
		.o_if_stall(w_if_stall),

		// Decode outputs
		.o_id_flags(w_id_flags),
		.o_id_a(w_id_a),
		.o_id_b(w_id_b),
		.o_id_adata(w_id_adata),
		.o_id_imm(w_id_imm),

		// EX stage feedback / control
		.i_ex_alu_result(w_ex_alu_result),
		.i_ex_adata(r_idex_adata_sel ? w_ex_alu_result : r_idex_adata),
		.i_ex_rf_we(r_idex_wb_rf_we),
		.i_ex_rf_wsel(r_idex_wb_rf_wsel),
		.i_ex_cf_we(r_idex_wb_cf_we),
		.i_ex_cf_flags_we(r_idex_wb_cf_flags_we),
		.i_ex_cf_sp_we(r_idex_wb_cf_sp_we),
		.i_ex_cf_wsel(r_idex_wb_cf_wsel),
		.i_ex_mem_re(r_idex_mem_re),
		.i_ex_flags({r_idex_flags[4], w_ex_alu_flags}),
		.o_ex_alu_a_sel(w_ex_alu_a_sel),
		.o_ex_alu_b_sel(w_ex_alu_b_sel),
		.o_ex_wdata_sel(w_ex_wdata_sel),
		.o_ex_adata_sel(w_ex_adata_sel),
		.o_ex_alu_op(w_ex_alu_op),
		.o_ex_adr_sel(w_ex_adr_sel),

		// MEM stage feedback / control
		.i_mem_flags(r_exmem_mem_re ?
				{r_exmem_flags[4], 1'bx, dmem_dat_i[7], dmem_dat_i == 0, 1'bx} :
				r_exmem_flags),
		.i_mem_rdata(r_exmem_mem_re ? w_mem_rdata : r_exmem_wdata),
		.i_mem_adata(r_exmem_adata),
		.i_mem_rf_we(r_exmem_wb_rf_we),
		.i_mem_rf_wsel(r_exmem_wb_rf_wsel),
		.i_mem_cf_we(r_exmem_wb_cf_we),
		.i_mem_cf_flags_we(r_exmem_wb_cf_flags_we),
		.i_mem_cf_sp_we(r_exmem_wb_cf_sp_we),
		.i_mem_cf_wsel(r_exmem_wb_cf_wsel),
		.o_mem_re(w_mem_re),
		.o_mem_we(w_mem_we),

		// WB stage feedback / control
		.i_wb_rdata(r_memwb_rdata),
		.i_wb_adata(r_memwb_adata),
		.i_wb_rf_we(r_memwb_rf_we),
		.i_wb_rf_wsel(r_memwb_rf_wsel),
		.i_wb_cf_we(r_memwb_cf_we),
		.i_wb_cf_flags_we(r_memwb_cf_flags_we),
		.i_wb_cf_sp_we(r_memwb_cf_sp_we),
		.i_wb_cf_wsel(r_memwb_cf_wsel),
		.i_wb_flags(r_memwb_flags),
		.o_wb_rf_we(w_wb_rf_we),
		.o_wb_rf_wsel(w_wb_rf_wsel),
		.o_wb_cf_we(w_wb_cf_we),
		.o_wb_cf_flags_we(w_wb_cf_flags_we),
		.o_wb_cf_sp_we(w_wb_cf_sp_we),
		.o_wb_cf_wsel(w_wb_cf_wsel),
		.o_wb_pc_lwe(w_wb_pc_lwe),
		.o_wb_pc_uwe(w_wb_pc_uwe),
		.o_wb_pc_we(w_wb_pc_we),

		// IFID hold
		.o_ifid_hold(w_ifid_hold)
	);

	// ID/EX Register
	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			r_idex_mem_re		<= 1'b0;
			r_idex_mem_we		<= 1'b0;
			r_idex_wb_rf_we		<= 1'b0;
			r_idex_wb_cf_we		<= 1'b0;
			r_idex_wb_cf_flags_we	<= 1'b0;
			r_idex_wb_cf_sp_we	<= 1'b0;
		end else if (!(dmem_stall_i || w_id_stall)) begin
			r_idex_npc		<= r_ifid_npc;
			r_idex_flags		<= w_id_flags;
			r_idex_a		<= w_id_a;
			r_idex_b		<= w_id_b;
			r_idex_adata		<= w_id_adata;
			r_idex_imm		<= w_id_imm;
			r_idex_alu_a_sel	<= w_ex_alu_a_sel;
			r_idex_alu_b_sel	<= w_ex_alu_b_sel;
			r_idex_alu_op		<= w_ex_alu_op;
			r_idex_wdata_sel	<= w_ex_wdata_sel;
			r_idex_adata_sel	<= w_ex_adata_sel;
			r_idex_adr_sel		<= w_ex_adr_sel;
			r_idex_mem_re		<= w_mem_re;
			r_idex_mem_we		<= w_mem_we;
			r_idex_wb_rf_we		<= w_wb_rf_we;
			r_idex_wb_rf_wsel	<= w_wb_rf_wsel;
			r_idex_wb_cf_we		<= w_wb_cf_we;
			r_idex_wb_cf_flags_we	<= w_wb_cf_flags_we;
			r_idex_wb_cf_sp_we	<= w_wb_cf_sp_we;
			r_idex_wb_cf_wsel	<= w_wb_cf_wsel;
			r_idex_wb_pc_lwe	<= w_wb_pc_lwe;
			r_idex_wb_pc_uwe	<= w_wb_pc_uwe;
			r_idex_wb_pc_we		<= w_wb_pc_we;
		end
	end

	// Execute stage
	alu u_alu (
		.i_a(r_idex_alu_a_sel == 1 ? r_idex_adata : (
			r_idex_alu_a_sel == 2 ? {r_idex_npc, 1'b0} : {8'b0, r_idex_a})),
		.i_b(r_idex_alu_b_sel ? r_idex_imm : {8'b0, r_idex_b}),
		.i_c(r_idex_flags[0]),
		.i_op(r_idex_alu_op),
		.o_result(w_ex_alu_result),
		.o_flags(w_ex_alu_flags)
	);
	assign w_ex_adr = r_idex_adr_sel ? w_ex_alu_result : r_idex_adata;

	// EX/MEM Register
	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			r_exmem_mem_re		<= 1'b0;
			r_exmem_mem_we		<= 1'b0;
			r_exmem_wb_rf_we	<= 1'b0;
			r_exmem_wb_cf_we	<= 1'b0;
			r_exmem_wb_cf_flags_we	<= 1'b0;
			r_exmem_wb_cf_sp_we	<= 1'b0;
		end else if (!dmem_stall_i) begin
			r_exmem_wdata		<= r_idex_wdata_sel ? r_idex_a : w_ex_alu_result[7:0];
			r_exmem_flags		<= {r_idex_flags[4], w_ex_alu_flags};
			r_exmem_adata		<= r_idex_adata_sel ? w_ex_alu_result : r_idex_adata;
			r_exmem_adr		<= w_ex_adr;
			r_exmem_mem_re		<= r_idex_mem_re;
			r_exmem_mem_we		<= r_idex_mem_we;
			r_exmem_wb_rf_we	<= r_idex_wb_rf_we;
			r_exmem_wb_rf_wsel	<= r_idex_wb_rf_wsel;
			r_exmem_wb_cf_we	<= r_idex_wb_cf_we;
			r_exmem_wb_cf_flags_we	<= r_idex_wb_cf_flags_we;
			r_exmem_wb_cf_sp_we	<= r_idex_wb_cf_sp_we;
			r_exmem_wb_cf_wsel	<= r_idex_wb_cf_wsel;
			r_exmem_wb_pc_lwe	<= r_idex_wb_pc_lwe;
			r_exmem_wb_pc_uwe	<= r_idex_wb_pc_uwe;
			r_exmem_wb_pc_we	<= r_idex_wb_pc_we;
		end
	end

	// Memory stage
	assign dmem_re_o  = r_exmem_mem_re;
	assign dmem_we_o  = r_exmem_mem_we;
	assign dmem_adr_o = r_exmem_adr;
	assign dmem_dat_o = r_exmem_wdata;

	// MEM/WB Register
	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			r_memwb_rf_we		<= 1'b0;
			r_memwb_cf_we		<= 1'b0;
			r_memwb_cf_flags_we	<= 1'b0;
			r_memwb_cf_sp_we	<= 1'b0;
		end else if (!dmem_stall_i) begin
			r_memwb_flags		<= r_exmem_mem_re ?
				{r_exmem_flags[4], 1'b0, dmem_dat_i[7], dmem_dat_i == 0, 1'b0} :
				r_exmem_flags;
			r_memwb_rdata		<= r_exmem_mem_re ? dmem_dat_i : r_exmem_wdata;
			r_memwb_adata		<= r_exmem_adata;
			r_memwb_rf_we		<= r_exmem_wb_rf_we;
			r_memwb_rf_wsel		<= r_exmem_wb_rf_wsel;
			r_memwb_cf_we		<= r_exmem_wb_cf_we;
			r_memwb_cf_flags_we	<= r_exmem_wb_cf_flags_we;
			r_memwb_cf_sp_we	<= r_exmem_wb_cf_sp_we;
			r_memwb_cf_wsel		<= r_exmem_wb_cf_wsel;
			r_memwb_pc_lwe		<= r_exmem_wb_pc_lwe;
			r_memwb_pc_uwe		<= r_exmem_wb_pc_uwe;
			r_memwb_pc_we		<= r_exmem_wb_pc_we;
		end
	end
endmodule