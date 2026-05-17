module cpu(
	// Wishbone signals
	input  logic        clk_i,
	input  logic        rst_i,
	input  logic        ack_i,
	input  logic        stall_i,
	input  logic [7:0]  dat_i,
	output logic	    stb_o,
	output logic	    cyc_o,
	output logic        we_o,
	output logic        lock_o,
	output logic        tgc_o,
	output logic [7:0]  dat_o,
	output logic [15:0] adr_o,

	// interrupt signals
	input  logic        i_int,
	input  logic [6:0]  i_int_id,
	output logic        o_int_ack
);

    	// registers
	logic [14:0] 	r_pc;
	logic [15:0] 	r_address;
	logic [15:0] 	r_data;
	// control signals
	logic [1:0]	w_ctr_alu_a_sel;
	logic [1:0]	w_ctr_alu_b_sel;
	logic		w_ctr_pc_we;
	logic [1:0]	w_ctr_address_sel;
	logic		w_ctr_address_we;
	logic		w_ctr_data_we;
	

	//Wishbone signals
	logic		w_wb_we;
	logic		w_wb_req;
	logic		w_wb_lock;
	logic [7:0]	w_wb_data_i;
	logic [15:0]	w_wb_addr;
	logic		w_wb_ready;
	logic [7:0]	w_wb_data_o;
	//Register File signals
	logic		w_rf_we;
	logic [2:0]     w_rf_rsel1;
	logic [2:0]     w_rf_rsel2;
	logic [2:0]     w_rf_wsel;
	logic [7:0]     w_rf_wdata;
	logic [7:0]     w_rf_rdata1;
	logic [7:0]     w_rf_rdata2;
	logic [15:0]    w_rf_adata;
	//CSR File signals
	logic		w_cf_we;
	logic [2:0]     w_cf_rsel;
	logic [2:0]     w_cf_wsel;
	logic [7:0]     w_cf_wdata;
	logic [15:0]    w_cf_sp_i;
	logic		w_cf_wsp;
	logic [7:0]     w_cf_rdata;
	logic [4:0]     w_cf_flags;
	logic [15:0]    w_cf_sp_o;
	//ALU signals
	logic [15:0]    w_alu_a;
	logic [15:0]    w_alu_b;
	logic [3:0]     w_alu_op;
	logic           w_alu_c;
	logic [15:0]    w_alu_result;
	logic [3:0]     w_alu_flags;
	//Imediate Generator signals
	logic [12:4]    w_ig_instr;
	logic [2:0]     w_ig_sel;
	logic [15:0]    w_ig_immediate;

	wb_master wb(
		.clk_i(clk_i),
		.rst_i(rst_i),
		.ack_i(ack_i),
		.stall_i(stall_i),
		.dat_i(dat_i),
		.stb_o(stb_o),
		.cyc_o(cyc_o),
		.we_o(we_o),
		.lock_o(lock_o),
		.tgc_o(tgc_o),
		.dat_o(dat_o),
		.adr_o(adr_o),
		.i_we(w_wb_we),
		.i_req(w_wb_req),
		.i_lock(w_wb_lock),
		.i_data(w_wb_data_i),
		.i_addr(w_wb_addr),
		.o_ready(w_wb_ready),
		.o_data(w_wb_data_o)
	);

	register_file rf(
		.i_clk(clk_i),
		.i_we(w_rf_we),
		.i_rsel1(w_rf_rsel1),
		.i_rsel2(w_rf_rsel2),
		.i_wsel(w_rf_wsel),
		.i_wdata(w_rf_wdata),
		.o_rdata1(w_rf_rdata1),
		.o_rdata2(w_rf_rdata2),
		.o_adata(w_rf_adata)
	);

	csr_file cf(
		.i_clk(clk_i),
		.i_rst(rst_i),
		.i_we(w_cf_we),
		.i_rsel(w_cf_rsel),
		.i_wsel(w_cf_wsel),
		.i_wdata(w_cf_wdata),
		.i_sp(w_cf_sp_i),
		.i_wsp(w_cf_wsp),
		.o_rdata(w_cf_rdata),
		.o_flags(w_cf_flags),
		.o_sp(w_cf_sp_o)
	);

	alu alu(
		.i_a(w_alu_a),
		.i_b(w_alu_b),
		.i_op(w_alu_op),
		.i_c(w_alu_c),
		.o_result(w_alu_result),
		.o_flags(w_alu_flags)
	);

	immediate_gen ig(
		.i_instr(w_ig_instr),
		.i_sel(w_ig_sel),
		.o_immediate(w_ig_immediate)
	);

	always_comb begin
		case (w_ctr_alu_a_sel)
			2'h0: w_alu_a = {8'b0, w_rf_rdata1};
			2'h1: w_alu_a = w_rf_adata;
			2'h2: w_alu_a = {r_pc, 1'b0};
			2'h3: w_alu_a = w_cf_sp_o;
		endcase
		case (w_ctr_alu_b_sel)
			2'h0: w_alu_b = {8'b0, w_rf_rdata2};
			2'h1: w_alu_b = w_ig_immediate;
			2'h2: w_alu_b = 16'h1;
			2'h3: w_alu_b = 16'h2;
		endcase
	end

	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			r_pc <= 15'b0;
			r_address <= 16'b0;
			r_data <= 16'b0;
		end else begin
			if (w_ctr_pc_we) begin
				r_pc <= w_alu_result[15:1];
			end
			if (w_ctr_address_we) begin
				case (w_ctr_address_sel)
					2'h0: r_address <= w_alu_result;
					2'h1: r_address <= w_cf_sp_o;
					2'h2: r_address <= {r_pc, 1'b0};
					2'h3: r_address <= {r_pc, 1'b1};
				endcase
			end
			if (w_ctr_data_we) begin
				r_data <= w_alu_result;
			end
		end
	end
endmodule