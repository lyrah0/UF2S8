`timescale 1ns / 1ps
module wb_master(
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
	// Cycle tags:
	// 0:single
	// 1:block
	output logic [7:0]  dat_o,
	output logic [15:0] adr_o,

	// CPU signals
	input  logic        i_we,
	input  logic        i_req,
	input  logic        i_lock,
	input  logic [7:0]  i_data,
	input  logic [15:0] i_addr,
	output logic        o_ready,
	output logic [7:0]  o_data
);
	always_comb begin
		dat_o = i_data;
		adr_o = i_addr;
		o_data = dat_i;
		o_ready = ack_i && stb_o;
	end

	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			stb_o  <= 1'b0;
			cyc_o  <= 1'b0;
			we_o   <= 1'b0;
			lock_o <= 1'b0;
			tgc_o  <= 1'b0;
		end else begin
			if (!cyc_o) begin
				if (i_req) begin
					cyc_o  <= 1'b1;
					stb_o  <= 1'b1;
					we_o   <= i_we;
					lock_o <= i_lock;
					tgc_o  <= i_lock;
				end
			end else begin
				// If request is active and acknowledged, complete it
				if (stb_o && ack_i) begin
					stb_o <= 1'b0;
					// If CPU wants another locked transfer, keep cycle open
					if (lock_o) begin
						cyc_o <= 1'b1;
					end else begin
						cyc_o <= 1'b0;
					end
				end else if (!stb_o) begin
					if (i_req) begin
						// CPU starts another locked request
						stb_o <= 1'b1;
						we_o  <= i_we;
						lock_o <= i_lock;
						tgc_o  <= i_lock;
					end else begin
						// No more requests, close the cycle
						cyc_o <= 1'b0;
						lock_o <= 1'b0;
						tgc_o <= 1'b0;
					end
				end
			end
		end
	end
	

endmodule