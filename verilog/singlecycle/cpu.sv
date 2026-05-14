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
	output logic		o_mem_we,
	// Interrupt Interface
	input logic 		i_interrupt,
	input logic [6:0] 	i_interrupt_id,
	output logic 		o_interrupt_ack

);

	// Signals
	logic [14:0]	r_pc;
	logic [14:0]	w_pc_next;
	logic		w_pc_we;
	logic [3:0]	w_id_op1;
	logic [2:0]	w_id_op2;
	logic [2:0]	w_id_rsel1;
	logic [2:0]	w_id_rsel2;
	logic [1:0]	w_id_asel;
	logic [2:0]	w_id_wsel;
	logic		w_id_opi;
	logic		w_id_opbr;
	logic [7:0]	w_alu_a;
	logic [7:0]	w_alu_b;
	logic [3:0]	w_alu_op;
	logic [7:0]	w_alu_result;
	logic [3:0]	w_alu_flags;
	logic [7:0]	w_rf_rdata1;
	logic [7:0]	w_rf_rdata2;
	logic [15:0]	w_rf_adata;
	logic [2:0]	w_rf_rsel1;
	logic [2:0]	w_rf_rsel2;
	logic [7:0]	w_rf_wdata;
	logic		w_rf_we;
	logic [7:0] 	w_cf_rdata;
	logic [7:0] 	w_cf_wdata;
	logic       	w_cf_we;
	logic [2:0] 	w_cf_rsel;
	logic [2:0] 	w_cf_wsel;
	logic       	w_cf_wsp;
	logic [15:0] 	w_cf_i_sp;
	logic [15:0] 	w_cf_o_sp;
	logic [7:0]  	w_cf_flags;
	logic [2:0]	w_ig_sel;
	logic [15:0]	w_ig_imm;
	logic [2:0]	w_agu_sel;
	logic [15:0]	w_agu_addr;
	logic [1:0]	w_sp_sel;
	logic [15:0]	w_sp_sp;
	logic [1:0]	w_pc_sel;
	logic		w_pc_rwe;
	logic [15:0]	w_pc_pc;
	logic		w_alu_b_sel;
	logic [1:0]	w_rf_wdata_sel;
	logic [1:0]	w_mem_wdata_sel;
	logic [1:0]	w_cf_wdata_sel;

	inst_decode u_inst_decode (
		.i_instr(i_instr),
		.o_op1(w_id_op1),
		.o_op2(w_id_op2),
		.o_rsel1(w_id_rsel1),
		.o_rsel2(w_id_rsel2),
		.o_asel(w_id_asel),
		.o_wsel(w_id_wsel),
		.o_opi(w_id_opi),
		.o_opbr(w_id_opbr)
	);

	alu u_alu (
		.i_a(w_alu_a),
		.i_b(w_alu_b),
		.i_c(w_cf_flags[0]),
		.i_op(w_alu_op),
		.o_result(w_alu_result),
		.o_flags(w_alu_flags)
	);

	register_file u_register_file (
		.i_clk(i_clk),
		.i_we(w_rf_we),
		.i_rsel1(w_rf_rsel1),
		.i_rsel2(w_rf_rsel2),
		.i_wsel(w_id_wsel),
		.i_wdata(w_rf_wdata),
		.o_rdata1(w_rf_rdata1),
		.o_rdata2(w_rf_rdata2),
		.o_adata(w_rf_adata)
	);

	csr_file u_csr_file (
		.i_clk(i_clk),
		.i_rst_n(i_rst_n),
		.i_we(w_cf_we),
		.i_rsel(w_cf_rsel),
		.i_wsel(w_cf_wsel),
		.i_wdata(w_cf_wdata),
		.i_sp(w_sp_sp),
		.i_wsp(w_cf_wsp),
		.o_rdata(w_cf_rdata),
		.o_flags(w_cf_flags),
		.o_sp(w_cf_o_sp)
	);

	immediate_gen u_immediate_gen (
		.i_instr(i_instr),
		.i_sel(w_ig_sel),
		.o_immediate(w_ig_imm)
	);
	
	agu u_agu (
		.i_sel(w_agu_sel),
		.i_offset(w_ig_imm),
		.i_baseaddr(w_rf_adata),
		.i_pc_next(w_pc_next),
		.i_sp(w_cf_o_sp),
		.o_addr(w_agu_addr)
	);
	
	spsel u_spsel (
		.i_sel(w_sp_sel),
		.i_sp(w_cf_o_sp),
		.o_sp(w_sp_sp)
	);
	
	pcsel u_pcsel (
		.i_clk(i_clk),
		.i_sel(w_pc_sel),
		.i_pc_next(w_pc_next),
		.i_offset(w_ig_imm),
		.i_addr(w_rf_adata),
		.i_data(i_mem_rdata),
		.i_we(w_pc_rwe),
		.o_pc(w_pc_pc)
	);

	control_unit u_control_unit (
		.i_clk(i_clk),
		.i_rst_n(i_rst_n),
		.i_instr(i_instr),
		.i_op1(w_id_op1),
		.i_op2(w_id_op2),
		.i_rsel1(w_id_rsel1),
		.i_rsel2(w_id_rsel2),
		.i_asel(w_id_asel),
		.i_wsel(w_id_wsel),
		.i_flags(w_cf_flags),
		.i_opi(w_id_opi),
		.i_opbr(w_id_opbr),
		.o_alu_op(w_alu_op),
		.o_rf_we(w_rf_we),
		.o_rf_rsel1(w_rf_rsel1),
		.o_rf_rsel2(w_rf_rsel2),
		.o_cf_we(w_cf_we),
		.o_cf_rsel(w_cf_rsel),
		.o_cf_wsel(w_cf_wsel),
		.o_cf_wsp(w_cf_wsp),
		.o_ig_sel(w_ig_sel),
		.o_agu_sel(w_agu_sel),
		.o_sp_sel(w_sp_sel),
		.o_pc_sel(w_pc_sel),
		.o_pc_rwe(w_pc_rwe),
		.o_pc_we(w_pc_we),
		.o_alu_b_sel(w_alu_b_sel),
		.o_rf_wdata_sel(w_rf_wdata_sel),
		.o_mem_wdata_sel(w_mem_wdata_sel),
		.o_mem_we(o_mem_we),
		.o_cf_wdata_sel(w_cf_wdata_sel)
	);




	always_comb begin
		w_alu_a = w_rf_rdata1;
		w_pc_next = r_pc + 1;

		o_pc = {r_pc,1'b0};
		o_mem_addr = w_agu_addr;

		case (w_alu_b_sel)
			1'b0: w_alu_b = w_rf_rdata2;
			1'b1: w_alu_b = w_ig_imm[7:0];
		endcase

		case (w_rf_wdata_sel)
			2'h0: w_rf_wdata = w_alu_result;
			2'h1: w_rf_wdata = w_cf_rdata;
			2'h2: w_rf_wdata = i_mem_rdata;
			default: w_rf_wdata = 8'bZ;
		endcase
		
		case (w_mem_wdata_sel)
			2'h0: o_mem_wdata = w_rf_rdata1;
			2'h1: o_mem_wdata = w_cf_rdata;
			2'h2: o_mem_wdata = {w_pc_next[6:0],1'b0};
			2'h3: o_mem_wdata = w_pc_next[14:7];
		endcase

		case (w_cf_wdata_sel)
			2'h0: w_cf_wdata = w_rf_rdata1;
			2'h1: w_cf_wdata = {w_cf_flags[7:4],w_alu_flags};
			2'h2: w_cf_wdata = {w_cf_flags[7:4],
				1'b0,
				w_rf_wdata[7],
				w_rf_wdata == 0,
				1'b0};
			default: w_cf_wdata = 8'bZ;
		endcase
	end

	always_ff @(posedge i_clk or negedge i_rst_n) begin
		if (!i_rst_n) begin
			r_pc <= 15'b0;
		end else begin
			if (w_pc_we) begin
				r_pc <= w_pc_pc[15:1];
			end
		end
	end
	
endmodule