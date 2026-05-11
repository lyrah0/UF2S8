module cpu (
	input  logic		i_clk,
	input  logic		i_rst_n,
	// Instruction Memory Interface
	output logic [15:0]	o_pc,
	input  logic [15:0]	i_instr,
	// Data Memory Interface
	output logic [15:0]	o_mem_addr,
	output logic [7:0]	o_mem_wdata,
	input  logic [7:0]	i_mem_rdata,
	output logic		o_mem_we
);

	// Registers
	logic [14:0]	r_pc;      // Program Counter

	// ALU
	logic [7:0]	w_alu_a;
	logic [7:0]	w_alu_b;
	logic		w_alu_c;
	logic [3:0]	w_alu_op;
	logic [7:0]	w_alu_result;
	logic [3:0]	w_alu_flags;

	alu u_alu (
		.i_a(w_alu_a),
		.i_b(w_alu_b),
		.i_c(w_alu_c),
		.i_op(w_alu_op),
		.o_result(w_alu_result),
		.o_flags(w_alu_flags)
	);

	// Register File
	logic [7:0] w_rf_rdata1;
	logic [7:0] w_rf_rdata2;
	logic [15:0] w_rf_adata;
	logic [2:0] w_rf_rsel1;
	logic [2:0] w_rf_rsel2;
	logic [2:0] w_rf_wsel;
	logic [7:0] w_rf_wdata;
	logic       w_rf_we;

	register_file u_register_file (
		.i_clk(i_clk),
		.i_rst_n(i_rst_n),
		.i_we(w_rf_we),
		.i_rsel1(w_rf_rsel1),
		.i_rsel2(w_rf_rsel2),
		.i_wsel(w_rf_wsel),
		.i_wdata(w_rf_wdata),
		.o_rdata1(w_rf_rdata1),
		.o_rdata2(w_rf_rdata2),
		.o_adata(w_rf_adata)
	);

	// CSR file
	logic [7:0] w_cf_rdata;
	logic [7:0] w_cf_wdata;
	logic       w_cf_we;
	logic [2:0] w_cf_rsel;
	logic [2:0] w_cf_wsel;
	logic       w_cf_wsp;
	logic [15:0] w_cf_i_sp;
	logic [15:0] w_cf_o_sp;

	csr_file u_csr_file (
		.i_clk(i_clk),
		.i_rst_n(i_rst_n),
		.i_we(w_cf_we),
		.i_rsel(w_cf_rsel),
		.i_wsel(w_cf_wsel),
		.i_wdata(w_cf_wdata),
		.i_sp(w_cf_i_sp),
		.i_wsp(w_cf_wsp),
		.o_rdata(w_cf_rdata),
		.o_sp(w_cf_o_sp)
	);

	// Immediate Generator
	logic [2:0] w_ig_sel;
	logic [15:0] w_ig_imm;

	immediate_gen u_immediate_gen (
		.i_instr(i_instr),
		.i_sel(w_ig_sel),
		.o_immediate(w_ig_imm)
	);
	
	// Address Generation Unit
	logic [2:0] w_agu_sel;
	logic [15:0] w_agu_addr;

	agu u_agu (
		.i_sel(w_agu_sel),
		.i_offset(w_ig_imm),
		.i_baseaddr(w_rf_adata),
		.i_pc({r_pc,1'b0}),
		.i_sp(w_sp_o_sp),
		.o_addr(w_agu_addr)
	);
	
	// Stack Pointer Selector
	logic [1:0] w_sp_sel;
	logic [15:0] w_sp_o_sp;

	spsel u_spsel (
		.i_sel(w_sp_sel),
		.i_sp(w_cf_o_sp),
		.o_sp(w_sp_o_sp)
	);
	
	// PC Selector
	logic [1:0] w_pc_sel;
	logic [15:0] w_pc_pc;

	pcsel u_pcsel (
		.i_sel(w_pc_sel),
		.i_pc(r_pc),
		.i_addr(w_agu_addr),
		.i_data(i_mem_rdata),
		.o_pc(w_pc_pc)
	);
	
endmodule