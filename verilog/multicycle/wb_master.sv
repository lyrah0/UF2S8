module wb_master(
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
	// Combinational assignments for address and data paths
	always_comb begin
		dat_o = i_data;
		adr_o = i_addr;
		o_data = dat_i;
		o_ready = ack_i;
	end

	// Internal tracking registers
	logic [1:0]	outstanding_cnt;
	logic [1:0]	next_outstanding_cnt;
	logic		is_second_req;
	logic		next_stb;

	// Calculate the next number of outstanding transactions combinationally
	// (stb_o && ~stall_i) means a request phase was accepted by the slave
	assign next_outstanding_cnt = outstanding_cnt + (stb_o && ~stall_i) - ack_i;
	
	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			stb_o <= 1'b0;
			cyc_o <= 1'b0;
			lock_o <= 1'b0;
			tgc_o <= 1'b0;
			outstanding_cnt <= 2'h0;
			is_second_req <= 1'b0;
			next_stb <= 1'b0;
		end else begin
			// Drive the CPU ready signal directly from the slave ack signal

			// Update outstanding transaction counter
			outstanding_cnt <= next_outstanding_cnt;

			if (~cyc_o) begin
				stb_o <= 1'b0;
				next_stb <= 1'b0;
				we_o <= 1'b0;
				lock_o <= 1'b0;
				tgc_o <= 1'b0;
				if (i_req) begin
					stb_o <= 1'b1;
					next_stb <= 1'b1;
					cyc_o <= 1'b1;
					we_o <= i_we;
					lock_o <= i_lock;
					tgc_o <= i_lock;
					is_second_req <= 1'b0;
				end
			end else begin
				if (stb_o) begin
					if (stall_i) begin
						// Slave is busy
						stb_o <= 1'b1;
						next_stb <= 1'b1;
					end else begin
						// Allow 2nd transfer only if CPU wants and haven't already started it
						if (i_req && !is_second_req) begin
							stb_o <= 1'b1;
							next_stb <= 1'b1;
							is_second_req <= 1'b1;
						end else begin
							stb_o <= 1'b0;
							next_stb <= 1'b0;
						end
					end
				end else begin
					next_stb <= 1'b0;
				end

				// cycle deassertion logic
				cyc_o <= (next_outstanding_cnt > 2'b0) || next_stb;
			end
		end
	end
	

endmodule