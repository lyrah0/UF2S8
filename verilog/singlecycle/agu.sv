module agu(
	input logic [2:0]	i_sel,
	input logic [15:0]	i_offset,
	input logic [15:0]	i_baseaddr,
	input logic [14:0]	i_pc_next,
	input logic [15:0]	i_sp,
	input logic [6:0]	i_interrupt_id,
	output logic [15:0]	o_addr
);

	always_comb begin
		case (i_sel)
			3'h0: o_addr = i_baseaddr;
			3'h1: o_addr = i_baseaddr + i_offset;
			3'h2: o_addr = {i_pc_next,1'b0} + i_offset;
			3'h3: o_addr = i_sp;
			3'h4: o_addr = i_sp + 1;
			3'h5: o_addr = i_sp - 1;
			3'h6: o_addr = {8'hFF, i_interrupt_id, 1'b0};
			3'h7: o_addr = {8'hFF, i_interrupt_id, 1'b1};
			default: o_addr = 16'bZ;
		endcase
	end
	


endmodule