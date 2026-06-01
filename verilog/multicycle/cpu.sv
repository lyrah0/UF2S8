`timescale 1ns / 1ps
module cpu(
	// Wishbone signals
	input  logic        clk_i,
	input  logic        rst_i,
	input  logic        ack_i,
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
	logic [7:0] 	r_data;
	logic [15:0]	r_instruction;
	logic		r_wb_we;
	logic		r_wb_req;
	logic		r_wb_lock;
	logic [6:0]	r_int_id;
	// control signals
	logic [1:0]	w_ctr_alu_a_sel;
	logic		w_ctr_alu_b_sel;
	logic		w_ctr_pc_we;
	logic [1:0]	w_ctr_pc_sel;
	logic [2:0]	w_ctr_address_sel;
	logic		w_ctr_address_we;
	logic		w_ctr_data_we;
	logic		w_ctr_instruction_lwe;
	logic		w_ctr_instruction_uwe;
	logic		w_ctr_int_id_we;
	logic		w_ctr_int_id_sel;
	logic [2:0]	w_ctr_cf_wdata_sel;
	logic [1:0]	w_ctr_rf_wdata_sel;
	logic [1:0]	w_ctr_data_sel;

	

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
	logic [15:4]    w_ig_instr;
	logic [2:0]     w_ig_sel;
	logic [15:0]    w_ig_immediate;
	//Control Unit signals
	logic		w_cu_wb_we;
	logic		w_cu_wb_req;
	logic		w_cu_wb_lock;
	logic [6:0]	w_cu_int_id;

	wb_master wb(
		.clk_i(clk_i),
		.rst_i(rst_i),
		.ack_i(ack_i),
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

	control_unit cu(
		.i_clk(clk_i),
		.i_rst(rst_i),
		.i_instruction(r_instruction),
		.i_flags(w_cf_flags),
		.o_wb_we(w_cu_wb_we),
		.o_wb_req(w_cu_wb_req),
		.o_wb_lock(w_cu_wb_lock),
		.i_wb_ready(w_wb_ready),
		.o_rf_we(w_rf_we),
		.o_cf_we(w_cf_we),
		.o_cf_rsel(w_cf_rsel),
		.o_cf_wsel(w_cf_wsel),
		.o_cf_wsp(w_cf_wsp),
		.o_alu_a_sel(w_ctr_alu_a_sel),
		.o_alu_b_sel(w_ctr_alu_b_sel),
		.o_alu_op(w_alu_op),
		.o_ig_sel(w_ig_sel),
		.o_ctr_pc_we(w_ctr_pc_we),
		.o_ctr_pc_sel(w_ctr_pc_sel),
		.o_ctr_address_sel(w_ctr_address_sel),
		.o_ctr_address_we(w_ctr_address_we),
		.o_ctr_data_we(w_ctr_data_we),
		.o_ctr_instruction_lwe(w_ctr_instruction_lwe),
		.o_ctr_instruction_uwe(w_ctr_instruction_uwe),
		.o_ctr_cf_wdata_sel(w_ctr_cf_wdata_sel),
		.o_ctr_rf_wdata_sel(w_ctr_rf_wdata_sel),
		.o_ctr_data_sel(w_ctr_data_sel),
		.i_interrupt(i_int),
		.o_interrupt_id(w_cu_int_id),
		.or_interrupt_ack(o_int_ack),
		.o_interrupt_id_we(w_ctr_int_id_we),
		.o_interrupt_id_sel(w_ctr_int_id_sel),
		.i_rdata(w_rf_rdata1[6:0])
	);

	always_comb begin
		// static assignments
		w_wb_we = r_wb_we;
		w_wb_req = r_wb_req;
		w_wb_lock = r_wb_lock;
		w_wb_addr = r_address;
		w_wb_data_i = r_data;
		w_rf_wsel = r_instruction[15:13];
		w_rf_rsel1 = r_instruction[12:10];
		w_rf_rsel2 = r_instruction[9:7];
		w_cf_sp_i = w_alu_result;
		w_alu_c = w_cf_flags[0];
		w_ig_instr = r_instruction[15:4];

		case (w_ctr_alu_a_sel)
			2'h0: w_alu_a = {8'b0, w_rf_rdata1};
			2'h1: w_alu_a = w_rf_adata;
			2'h2: w_alu_a = {r_pc, 1'b0};
			2'h3: w_alu_a = w_cf_sp_o;
		endcase
		case (w_ctr_alu_b_sel)
			1'h0: w_alu_b = {8'b0, w_rf_rdata2};
			1'h1: w_alu_b = w_ig_immediate;
		endcase
		case (w_ctr_cf_wdata_sel)
			3'h0: w_cf_wdata = {w_cf_flags[4], 3'hx, w_alu_flags};
			3'h1: w_cf_wdata = w_rf_rdata1[7:0];
			3'h2: w_cf_wdata = w_wb_data_o;
			3'h3: w_cf_wdata = 8'h0;
			3'h4: w_cf_wdata = {w_cf_flags[4], 4'hx, w_wb_data_o[7], w_wb_data_o == 0, 1'hx};
			default: w_cf_wdata = 8'hx;
		endcase
		case (w_ctr_rf_wdata_sel)
			2'h0: w_rf_wdata = w_alu_result[7:0];
			2'h1: w_rf_wdata = w_wb_data_o;
			2'h2: w_rf_wdata = w_cf_rdata;
			default: w_rf_wdata = 8'hx;
		endcase
	end

	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			r_pc <= 15'b0;
			r_address <= 16'b0;
			r_data <= 8'b0;
		end else begin
			if (w_ctr_pc_we) begin
				case (w_ctr_pc_sel)
					2'h0: r_pc <= w_alu_result[15:1];
					2'h1: r_pc <= w_rf_adata[15:1];
					2'h2: r_pc <= {8'hx, w_wb_data_o[7:1]};
					2'h3: r_pc <= {w_wb_data_o, r_pc[6:0]};
					default: r_pc <= 15'bx;
				endcase
			end
			if (w_ctr_address_we) begin
				case (w_ctr_address_sel)
					3'h0: r_address <= w_alu_result;
					3'h1: r_address <= w_cf_sp_o;
					3'h2: r_address <= {r_pc, 1'b0};
					3'h3: r_address <= {r_pc, 1'b1};
					3'h4: r_address <= {8'hFF, r_int_id, 1'b0};
					3'h5: r_address <= {8'hFF, r_int_id, 1'b1};
					default: r_address <= 16'hx;
				endcase
			end
			r_wb_we <= w_cu_wb_we;
			r_wb_lock <= w_cu_wb_lock;
			r_wb_req <= w_cu_wb_req;
			if (w_ctr_data_we) begin
				case (w_ctr_data_sel)
					2'h0: r_data <= w_rf_rdata1;
					2'h1: r_data <= {w_cf_flags[4], 3'hx, w_cf_flags[3:0]};
					2'h2: r_data <= {r_pc[6:0], 1'b0};
					2'h3: r_data <= r_pc[14:7];
					default: r_data <= 8'bx;
				endcase
			end
			if (w_ctr_instruction_lwe) begin
				r_instruction[7:0] <= w_wb_data_o;
			end
			if (w_ctr_instruction_uwe) begin
				r_instruction[15:8] <= w_wb_data_o;
			end
			if (w_ctr_int_id_we) begin
				case(w_ctr_int_id_sel)
					1'b0: r_int_id <= i_int_id;
					1'b1: r_int_id <= w_cu_int_id;
				endcase
			end
		end
	end
endmodule