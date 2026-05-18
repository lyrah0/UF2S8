module wb_ram_slave(
	input  logic        clk_i,
	input  logic        cyc_i,
	input  logic        stb_i,
	input  logic        we_i,
	input  logic [7:0]  dat_i,
	input  logic [15:0] adr_i,
	output logic	    ack_o,
	output logic [7:0]  dat_o
);

	logic [7:0] ram [0:65535];

	assign ack_o   = cyc_i && stb_i;
	assign dat_o   = ram[adr_i];

	always_ff @(posedge clk_i) begin
		if (cyc_i && stb_i && we_i) begin
			ram[adr_i] <= dat_i;
		end
	end

endmodule

