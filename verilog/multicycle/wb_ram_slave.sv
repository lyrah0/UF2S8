module wb_ram_slave(
	input  logic        clk_i,
	input  logic        rst_i,
	input  logic        cyc_i,
	input  logic        stb_i,
	input  logic        we_i,
	input  logic [7:0]  dat_i,
	input  logic [15:0] adr_i, 
	output logic	    stall_o,
	output logic	    ack_o,
	output logic [7:0]  dat_o
);

	logic [7:0] ram [0:65535];

	logic		latency_pipeline;
	logic		ram_re = cyc_i && stb_i && ~we_i;
	logic		ram_we = cyc_i && stb_i && we_i;
	
	assign stall_o = 1'b0;

	always_ff @(posedge clk_i) begin
		if (rst_i) begin
			latency_pipeline <= 1'b0;
			ack_o <= 1'b0;
		end else begin
			if (cyc_i) begin
				latency_pipeline <= ram_re;
				ack_o <= ram_we || latency_pipeline;
			end else begin
				ack_o <= 1'b0;
				latency_pipeline <= 1'b0;
			end
		end
		if (ram_re) begin
			dat_o <= ram[adr_i];
		end
		if (ram_we) begin
			ram[adr_i] <= dat_i;
		end
	end

	


endmodule
